#include "log.h"

void Log::AddTrans(const std::string &counterparty_str, bool receiving, uint32_t amount, time_t time) noexcept
{
#if USE_DEPRECATED_ENDPOINTS
    log_flag.SetChangesOn();
#endif
    log_flag_v2.SetChangesOn();

    if (data.size() == MAX_LOG_SIZE)
    {
        data.pop_front();
    }
    data.emplace_back(counterparty_str, receiving, amount, time);
}

#if USE_DEPRECATED_ENDPOINTS
std::string Log::GetLogs(const std::string& name) noexcept
{
    if (log_flag.GetChangeState() && data.size()) // if there are changes
    {
        log_snapshot.resize(0);
        //re-generate snapshot
        size_t predicted_size = ((57 + (2 * max_name_size)) * data.size()) + 1;
        if (log_snapshot.capacity() < predicted_size)
        {
            log_snapshot.reserve(predicted_size);
        }
        log_snapshot = '['; //1
        for (size_t i = 0; i < data.size(); ++i)
        {
            log_snapshot += "{\"to\":\"";                                                       //7
            log_snapshot += data[i].receiving? name : data[i].counterparty;                     //max_name_size?
            log_snapshot += "\",\"from\":\"";                                                   //10
            log_snapshot += data[i].receiving? data[i].counterparty : name;                     //max_name_size?
            log_snapshot += "\",\"amount\":";                                                   //11
            log_snapshot += std::to_string(data[i].amount);                                     //10?
            log_snapshot += ",\"time\":";                                                       //8
            log_snapshot += std::to_string(data[i].time);                                       //10?
            log_snapshot += "},";                                                               //2
        }
        log_snapshot.back() = ']';
        log_flag.SetChangesOff();
    }

    return log_snapshot;
}
#endif

std::string Log::GetLogsV2() noexcept
{
    if (log_flag_v2.GetChangeState() && data.size()) // if there are changes
    {
        log_snapshot_v2.resize(0);
        //re-generate snapshot
        size_t predicted_size = ((59 + max_name_size) * data.size()) + 1;
        if (log_snapshot_v2.capacity() < predicted_size)
        {
            log_snapshot_v2.reserve(predicted_size);
        }
        log_snapshot_v2 = '['; //1
        for (size_t i = data.size(); i --> 0;)
        {
            log_snapshot_v2 += "{\"counterparty\":\"";                                             //17
            log_snapshot_v2 += data[i].counterparty;                                               //max_name_size?
            log_snapshot_v2 += "\",\"amount\":";                                                   //11
            if (!data[i].receiving) { log_snapshot_v2 += '-'; }                                    //1
            log_snapshot_v2 += std::to_string(data[i].amount);                                     //10?
            log_snapshot_v2 += ",\"time\":";                                                       //8
            log_snapshot_v2 += std::to_string(data[i].time);                                       //10?
            log_snapshot_v2 += "},";                                                               //2
        }
        log_snapshot_v2.back() = ']';
        log_flag_v2.SetChangesOff();
    }

    return log_snapshot_v2;
}

std::string Log::GetLogsRange(size_t start, size_t length) noexcept
{
    if (start >= data.size()) { return "[]"; }
    if (start == 0 && length == MAX_LOG_SIZE) { return log_snapshot_v2; }
    if (log_flag_v2.GetChangeState() && data.size()) { GetLogsV2(); }

    size_t log_index_n, i;
    if (start < (0.5 * MAX_LOG_SIZE))
    {
        // std::cout << "a\n";
        i = 0;
        log_index_n = 0;
        while(i < log_snapshot_v2.size())
        {
            if (log_index_n == start)
            {
                log_index_n = i;
                break;
            }
            i += (41 + min_name_size);
            while (log_snapshot_v2[i] != ',') { ++i; }
            ++log_index_n;
        }
    }
    else
    {
        // std::cout << "b\n";
        i = log_snapshot_v2.size();
        log_index_n = data.size();
        while(i --> 0)
        {
            if (log_index_n == start)
            {
                log_index_n = i + 1;
                break;
            }
            i -= (41 + min_name_size);
            while (log_snapshot_v2[i] != ',') { --i; }
            --log_index_n;
        }
    }

    size_t log_index_m = std::string::npos;
    if ((start + length) < data.size())
    {
        if (length < (0.5 * MAX_LOG_SIZE))
        {
            // std::cout << "c\n";
            log_index_m = 0;
            while(i < log_snapshot_v2.size())
            {
                if (log_index_m == length)
                {
                    log_index_m = i + 1;
                    break;
                }
                i += (41 + min_name_size);
                while (log_snapshot_v2[i] != ',') { ++i; }
                ++log_index_m;
            }
        }
        else
        {
            // std::cout << "d\n";
            i = log_snapshot_v2.size();
            log_index_m = data.size();
            while(i --> 0)
            {
                if (log_index_m == length)
                {
                    log_index_m = i + 1;
                    break;
                }
                i -= (41 + min_name_size);
                while (log_snapshot_v2[i] != ',') { --i; }
                --log_index_m;
            }
        }
        
        log_index_m -= log_index_n;
    }

    std::string res(log_snapshot_v2.substr(log_index_n, log_index_m));
    res[0] = '[';
    res[res.size() - 1] = ']';
    // std::cout << res << '\n';
    
    return res;
}
