#include "Receiver.hpp"
#include "ErrorCode.hpp"
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>

ErrorCode Receiver::set_notifier(INotifier* notifier) {
    if (notifier_) {
        return ErrorCode::NOTIFIER_NOT_SET;
    }
    if (notifier) {
        notifier_ = notifier;
    }
    return ErrorCode::OK;
}

ErrorCode Receiver::subscribe(const std::string& topic) {
    if (notifier_) {
        if (std::find(subscribed_topics.cbegin(), subscribed_topics.cend(), topic) == subscribed_topics.cend()) {
            subscribed_topics.push_back(topic);
            ErrorCode err;
            err = notifier_->attach(this, topic);
            if (err != ErrorCode::OK) {
                return err;
            }
        } else {
            return ErrorCode::OBSERVER_ALREADY_ATTACHED;
        }
    } else {
        return ErrorCode::NOTIFIER_NOT_SET;
    }
    return ErrorCode::OK;
}

ErrorCode Receiver::unsubscribe(const std::string& topic) {
    if (notifier_) {
        auto it = std::find(subscribed_topics.cbegin(), subscribed_topics.cend(), topic);
        if (it != subscribed_topics.cend()) {
            subscribed_topics.erase(it);
            ErrorCode err;
            err = notifier_->detach(this, topic);
            if (err != ErrorCode::OK) {
                return err;
            }
        } else {
            return ErrorCode::OBSERVER_ALREADY_DETACHED;
        }
    } else {
        return ErrorCode::NOTIFIER_NOT_SET;
    }
    return ErrorCode::OK;
}

ErrorCode Receiver::update(const std::string& topic, const nlohmann::json& data) {
    if (topic.empty()) {
        return ErrorCode::UPDATE_TOPIC_EMPTY;
    } else if (data.empty()) {
        return ErrorCode::UPDATE_DATA_EMPTY;
    }
    std::cout << "Topic: " << topic << '\n';
    std::cout << "Data: " << data.dump() << '\n';
    return ErrorCode::OK;
}
