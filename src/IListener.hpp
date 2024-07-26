#pragma once
#include "build/_deps/json-src/include/nlohmann/json.hpp"
#include "src/Network/ErrorCode.hpp"

class IListener {
public:
    virtual ~IListener() = default;
    virtual ErrorCode update(const std::string& topic, const nlohmann::json& data) = 0;
};
