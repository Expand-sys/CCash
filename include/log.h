#pragma once
#include <json/json.h> // to be removed later
#include <vector>
#include <algorithm>
#include "ccash_config.hpp"
#include "change_flag.h"
#include "transaction.h"

struct Log
{
private:
    ChangeFlag<true> log_flag;
    std::string log_snapshot;

public:
#if MAX_LOG_SIZE == 1
    Transaction data;
#else
    std::vector<Transaction> data;
#endif

    const std::string &GetLogs() noexcept;
    void AddTrans(Transaction &&t) noexcept;
    Json::Value Serialize() const; // to be removed later
};
