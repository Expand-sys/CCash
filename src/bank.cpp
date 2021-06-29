#include "bank.h"

__attribute__((always_inline)) inline bool ValidUsrname(const std::string &name) noexcept
{
    if (name.size() < min_name_size || name.size() > max_name_size)
    {
        return false;
    }
    for (const char &c : name)
    {
        if (!(std::isalpha(c) || std::isdigit(c) || c == '_'))
        {
            return false;
        }
    }
    return true;
}

using namespace drogon;

bool Bank::GetChangeState() const noexcept { return save_flag.GetChangeState(); }

BankResponse Bank::GetBal(const std::string &name) const noexcept
{
    uint32_t res = 0;
    users.if_contains(name, [&res](const User &u) {
        res = u.balance + 1;
    });
    return res > 0 ? BankResponse(k200OK, res - 1) : BankResponse(k404NotFound, "User not found");
}
BankResponse Bank::GetLogs(const std::string &name) noexcept
{
    BankResponse res;
#if MAX_LOG_SIZE > 0
    if (!users.modify_if(name, [&res](User &u) {
            res = {k200OK, u.log.GetLog()};
        }))
    {
        return BankResponse(k404NotFound, "User not found");
    }
#endif
    return res;
}
BankResponse Bank::SendFunds(const std::string &a_name, const std::string &b_name, uint32_t amount) noexcept
{
    //cant send money to self, from self or amount is 0
    if (a_name == b_name)
    {
        return {k400BadRequest, "Sender and Reciever names cannot match"};
    }

    //cant send 0
    if (!amount)
    {
        return {k400BadRequest, "Amount being sent cannot be 0"};
    }

    //as first modify_if checks a_name and grabs unique lock
    if (!Contains(b_name))
    {
        return {k404NotFound, "Reciever does not exist"};
    }

    BankResponse state;
    std::shared_lock<std::shared_mutex> lock{send_funds_l}; //about 10% of this function's cost
#if MAX_LOG_SIZE > 0
    Transaction temp(a_name, b_name, amount);
    if (!users.modify_if(a_name, [&temp, &state, amount](User &a) {
#else
    if (!users.modify_if(a_name, [&state, amount](User &a) {
#endif
            //if A can afford it
            if (a.balance < amount)
            {
                state = {k400BadRequest, "Sender has insufficient funds"};
            }
            else
            {
                a.balance -= amount;
#if MAX_LOG_SIZE > 0
                a.log.AddTrans(Transaction(temp)); //about 40% of this function's cost
#endif
                state = {k200OK, "Transfer successful!"};
            }
        }))
    {
        return {k404NotFound, "Sender does not exist"};
    }
    if (state.first == k200OK)
    {
#if MAX_LOG_SIZE > 0
        users.modify_if(b_name, [&temp, amount](User &b) {
            b.balance += amount;
            b.log.AddTrans(std::move(temp)); }); //about 40% of this function's cost
#else
        users.modify_if(b_name, [amount](User &b) { b.balance += amount; });
#endif

#if CONSERVATIVE_DISK_SAVE
        save_flag.SetChangesOn(); //about 5% of this function's cost
#endif
    }
    return state;
}
bool Bank::VerifyPassword(const std::string &name, const std::string &attempt) const noexcept
{
    bool res = false;
    users.if_contains(name, [&res, &attempt](const User &u) {
        res = (u.password == XXH3_64bits(attempt.data(), attempt.size()));
    });
    return res;
}

void Bank::ChangePassword(const std::string &name, std::string &&new_pass) noexcept
{
    users.modify_if(name, [&new_pass](User &u) {
        u.password = XXH3_64bits(new_pass.data(), new_pass.size());
    });
#if CONSERVATIVE_DISK_SAVE
    save_flag.SetChangesOn();
#endif
}
BankResponse Bank::SetBal(const std::string &name, uint32_t amount) noexcept
{
    if (users.modify_if(name, [amount](User &u) {
            u.balance = amount;
        }))
    {
#if CONSERVATIVE_DISK_SAVE
        save_flag.SetChangesOn();
#endif
        return BankResponse(k200OK, "Balance set!");
    }
    else
    {
        return BankResponse(k404NotFound, "User not found");
    }
}
bool Bank::Contains(const std::string &name) const noexcept
{
    return users.contains(name);
}
bool Bank::AdminVerifyPass(const std::string &attempt) noexcept
{
    return (admin_pass == attempt);
}

BankResponse Bank::AddUser(const std::string &name, std::string &&init_pass) noexcept
{
    if (!ValidUsrname(name))
    {
        return BankResponse(k400BadRequest, "Invalid Name, breaks size and/or character restrictions");
    }

    std::shared_lock<std::shared_mutex> lock{size_l};
    return (users.try_emplace_l(
               std::move(name), [](User &) {}, std::move(init_pass)))
               ? BankResponse(k200OK, "User added!")
               : BankResponse(k409Conflict, "User already exists");
}
BankResponse Bank::AdminAddUser(std::string &&name, uint32_t init_bal, std::string &&init_pass) noexcept
{
    if (!ValidUsrname(name))
    {
        return BankResponse(k400BadRequest, "Invalid Name, breaks size and/or character restrictions");
    }

    std::shared_lock<std::shared_mutex> lock{size_l};
    return (users.try_emplace_l(
               std::move(name), [](User &) {}, init_bal, std::move(init_pass)))
               ? BankResponse(k200OK, "User added!")
               : BankResponse(k409Conflict, "User already exists");
}
BankResponse Bank::DelUser(const std::string &name) noexcept
{
    std::shared_lock<std::shared_mutex> lock{size_l};
#if RETURN_ON_DEL
    uint32_t bal;
    if (users.erase_if(name, [this, &bal, &name](User &u) {
            bal = u.balance;
            return true;
        }))
#else
    if (!users.erase(name))
#endif
    {
        return BankResponse(k404NotFound, "User not found");
    }
#if RETURN_ON_DEL
    users.modify_if(return_account, [&bal](User &u) {
        u.balance += bal;
    });
#endif
    return BankResponse(k200OK, "User deleted!");
}
void Bank::Save()
{
#if CONSERVATIVE_DISK_SAVE
    if (save_flag.GetChangeState())
    {
#endif
        Json::Value temp;

        //loading info into json temp
        {
            std::scoped_lock<std::shared_mutex, std::shared_mutex> lock{size_l, send_funds_l};
            for (const auto &u : users)
            {
                //we know it contains this key but we call this func to grab mutex
                users.if_contains(u.first, [&temp, &u](const User &u_val) {
                    temp[u.first] = u_val.Serialize();
                });
            }
        }
        if (temp.isNull())
        {
            throw std::invalid_argument("Saving Failed\n");
        }
        else
        {
            std::ofstream user_save(users_location);
            Json::StreamWriterBuilder builder;
            const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
            writer->write(temp, &user_save);
            user_save.close();
        }
#if CONSERVATIVE_DISK_SAVE
        save_flag.SetChangesOff();
    }
#endif
}

//NOT THREAD SAFE, BY NO MEANS SHOULD THIS BE CALLED WHILE RECEIEVING REQUESTS
void Bank::Load()
{
    Json::CharReaderBuilder builder;

    Json::Value temp;
    std::ifstream user_save(users_location);
    builder["collectComments"] = true;
    JSONCPP_STRING errs;
    if (!parseFromStream(builder, user_save, &temp, &errs))
    {
        std::cerr << errs << '\n';
        user_save.close();
        throw std::invalid_argument("Parsing Failed\n");
    }
    else
    {
        user_save.close();
        for (const auto &u : temp.getMemberNames())
        {
            if constexpr (MAX_LOG_SIZE > 0)
            {
                users.try_emplace(u, temp[u]["balance"].asUInt(), std::move(temp[u]["password"].asUInt64()), temp[u]["log"]);
            }
            else
            {
                users.try_emplace(u, temp[u]["balance"].asUInt(), std::move(temp[u]["password"].asUInt64()));
            }
        }
    }
}
