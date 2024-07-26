#pragma once
#include "build/_deps/json-src/include/nlohmann/json.hpp"
#include "src/IListener.hpp"
#include "src/Network/ErrorCode.hpp"
#include <string>

class INotifier {
public:
    virtual ~INotifier() = default;
    virtual ErrorCode attach(IListener* listener, const std::string& topic) noexcept = 0;
    virtual ErrorCode detach(IListener* listener, const std::string& topic) noexcept = 0;
    virtual ErrorCode notify(const std::string& topic, const nlohmann::json& data) noexcept = 0;
};
