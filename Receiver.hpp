#pragma once
#include "ErrorCode.hpp"
#include "IListener.hpp"
#include "INotifier.hpp"
#include <string>
#include <vector>

class Receiver : IListener {
public:
    ~Receiver() override = default;
    ErrorCode set_notifier(INotifier* notifier);
    ErrorCode subscribe(const std::string& topic);
    ErrorCode unsubscribe(const std::string& topic);
    ErrorCode update(const std::string& topic, const nlohmann::json& data) override;

private:
    INotifier* notifier_ {nullptr};
    std::vector<std::string> subscribed_topics;
};