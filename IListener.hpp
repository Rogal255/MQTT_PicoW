#pragma once
#include <nlohmann/json.hpp>
#include "ErrorCode.hpp"

class IListener {
public:
    virtual ~IListener() = default;
    virtual ErrorCode update(const std::string& topic, const nlohmann::json& data) = 0;
};
