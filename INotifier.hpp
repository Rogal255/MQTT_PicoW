#pragma once
#include "IListener.hpp"
#include <string>
#include <nlohmann/json.hpp>
#include "ErrorCode.hpp"

class INotifier {
public:
    virtual ~INotifier() = default;
    virtual ErrorCode attach(IListener* listener, const std::string& topic) noexcept = 0;
    virtual ErrorCode detach(IListener* listener, const std::string& topic) noexcept = 0;
    virtual ErrorCode notify(const std::string& topic, const nlohmann::json& data) noexcept = 0;
};
