#include "Network.hpp"
#include "ErrorCode.hpp"
#include "NetworkConfig.hpp"
#include <algorithm>
#include <iostream>
#include <memory>

ErrorCode Network::attach(IListener* listener, const std::string& topic) noexcept {
    auto it = std::find(observers_.cbegin(), observers_.cend(), std::make_pair<>(listener, topic));
    if (it != observers_.cend()) {
        return ErrorCode::OBSERVER_ALREADY_ATTACHED;
    }
    auto it_topic = std::find_if(observers_.cbegin(),
                                 observers_.cend(),
                                 [&](const std::pair<IListener*, std::string>& pair) { return pair.second == topic; });
    if (it_topic == observers_.cend()) {
        ErrorCode err;
        err = this->subscribe(topic, Subscription::SUBSCRIBE, 0);
        if (err != ErrorCode::OK) {
            return err;
        }
    }
    this->observers_.push_back(std::make_pair<>(listener, topic));
    return ErrorCode::OK;
}

ErrorCode Network::detach(IListener* listener, const std::string& topic) noexcept {
    auto it = std::find(observers_.cbegin(), observers_.cend(), std::make_pair<>(listener, topic));
    if (it == observers_.cend()) {
        return ErrorCode::OBSERVER_ALREADY_DETACHED;
    }
    observers_.erase(it);
    auto it_topic = std::find_if(observers_.cbegin(),
                                 observers_.cend(),
                                 [&](const std::pair<IListener*, std::string>& pair) { return pair.second == topic; });
    if (it_topic == observers_.cend()) {
        ErrorCode err;
        err = this->subscribe(topic, Subscription::UNSUBSCRIBE, 0);
        if (err != ErrorCode::OK) {
            return err;
        }
    }
    return ErrorCode::OK;
}

ErrorCode Network::notify(const std::string& topic, const nlohmann::json& data) noexcept {
    if (topic.empty()) {
        return ErrorCode::UPDATE_TOPIC_EMPTY;
    } else if (data.empty()) {
        return ErrorCode::UPDATE_DATA_EMPTY;
    }
    for (const auto& pair : observers_) {
        if (pair.second == topic) {
            pair.first->update(topic, data);
        }
    }
    return ErrorCode::OK;
}

ErrorCode Network::init() {
    auto err = wifi_connect();
    if (err != ErrorCode::OK) {
        return err;
    }

    err = mqtt_client_init();
    if (err != ErrorCode::OK) {
        return err;
    }

    err = dns_lookup();
    if (err != ErrorCode::OK) {
        return err;
    }

    err = broker_connect();
    if (err != ErrorCode::OK) {
        return err;
    }

    mqtt_set_inpub_callback(
        this->state_->mqtt_client, NetworkHelpers::mqtt_pub_start_cb, NetworkHelpers::mqtt_pub_data_cb, this);

    return ErrorCode::OK;
}

ErrorCode
Network::publish(const std::string& buffer, const std::string& topic, const uint8_t qos, const uint8_t retain) {
    if (buffer.empty()) {
        return ErrorCode::MQTT_PUBLISH_BUFFER_EMPTY;
    } else if (topic.empty()) {
        return ErrorCode::MQTT_PUBLISH_TOPIC_EMPTY;
    }
    cyw43_arch_lwip_begin();
    err_t err = mqtt_publish(
        this->state_->mqtt_client, topic.c_str(), buffer.c_str(), buffer.size(), qos, retain, nullptr, nullptr);
    cyw43_arch_lwip_end();
    if (err != ERR_OK) {
        return ErrorCode::MQTT_PUBLISH_ERROR;
    }
    return ErrorCode::OK;
}

void Network::loop() { cyw43_arch_poll(); }

ErrorCode Network::wifi_connect() {
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_POLAND)) {
        return ErrorCode::WIFI_NOT_INITIALIZED;
    }
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(
            NetworkConfig::SSID.c_str(), NetworkConfig::PASS.c_str(), CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        return ErrorCode::WIFI_NOT_CONNECTED;
    }
    return ErrorCode::OK;
}

ErrorCode Network::mqtt_client_init() {
    this->state_ = std::make_unique<MQTT_CLIENT_T>();
    if (!this->state_) {
        return ErrorCode::STATE_NOT_ALLOCATED;
    }
    if (!(this->state_->mqtt_client = mqtt_client_new())) {
        return ErrorCode::MQTT_CLIENT_NOT_CREATED;
    }
    return ErrorCode::OK;
}

ErrorCode Network::dns_lookup() {
    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(
        NetworkConfig::MQTT_SERVER_HOST.c_str(),
        &(this->state_->remote_addr),
        [](const char*, const ip_addr_t* ipaddr, void* usr_arg) {
            static_cast<MQTT_CLIENT_T*>(usr_arg)->remote_addr = *ipaddr;
        },
        this->state_.get());
    cyw43_arch_lwip_end();

    if (err == ERR_ARG) {
        return ErrorCode::DNS_CLIENT_NOT_INITIALIZED_OR_INVALID_HOSTNAME;
    } else if (err == ERR_OK) {
        return ErrorCode::OK;
    }

    while (this->state_->remote_addr.addr == 0) {
        cyw43_arch_poll();
        sleep_ms(1);
    }
    return ErrorCode::OK;
}

ErrorCode Network::broker_connect() {
    auto tls_config = altcp_tls_create_config_client((const u8_t*)NetworkConfig::CA_CERTIFICATE.c_str(),
                                                     NetworkConfig::CA_CERTIFICATE.size() + 1);
    if (!tls_config) {
        return ErrorCode::TLS_CONFIG_NOT_INITIALIZED;
    }

    mqtt_connect_client_info_t ci {.client_id = this->client_id_.c_str(),
                                   .client_user = NetworkConfig::CLIENT_USER.c_str(),
                                   .client_pass = NetworkConfig::CLIENT_PASS.c_str(),
                                   .keep_alive = 0,
                                   .will_topic = nullptr,
                                   .will_msg = nullptr,
                                   .will_qos = 0,
                                   .will_retain = 0,
                                   .tls_config = tls_config,
                                   .server_name = NetworkConfig::MQTT_SERVER_HOST.c_str()};

    err_t err = mqtt_client_connect(
        this->state_->mqtt_client,
        &(this->state_->remote_addr),
        NetworkConfig::MQTT_SERVER_PORT,
        [](mqtt_client_t*, void*, mqtt_connection_status_t status) {
            if (status == MQTT_CONNECT_ACCEPTED) {
                std::cout << "MQTT Connected\n";
            } else {
                std::cout << "Problem with MQTT connection: " << status << "\n";
            }
        },
        nullptr,
        &ci);

    if (err != ERR_OK) {
        return ErrorCode::TLS_NOT_CONNECTED;
    }
    return ErrorCode::OK;
}

ErrorCode Network::subscribe(const std::string& topic, const Subscription sub, const uint8_t qos) {
    cyw43_arch_lwip_begin();
    err_t err = mqtt_sub_unsub(
        this->state_->mqtt_client,
        topic.c_str(),
        qos,
        [](void*, err_t err) {
            if (err != ERR_OK) {
                std::cout << "Subscription error: " << static_cast<int>(err) << '\n';
            }
        },
        nullptr,
        static_cast<u8_t>(sub));
    cyw43_arch_lwip_end();
    if (err != ERR_OK) {
        return ErrorCode::MQTT_SUBSCRIPTION_ERROR;
    }
    return ErrorCode::OK;
}

static void NetworkHelpers::mqtt_pub_start_cb(void* arg, const char* topic, u32_t tot_len) {
    auto network {static_cast<Network*>(arg)};
    network->buffer_incoming_.reserve(tot_len);
    network->last_received_topic_ = topic;
    network->data_incoming_len_ = tot_len;
}

static void NetworkHelpers::mqtt_pub_data_cb(void* arg, const u8_t* data, u16_t len, u8_t flags) {
    auto network {static_cast<Network*>(arg)};
    for (uint32_t i {0}; i < len; ++i) {
        network->buffer_incoming_.push_back(*(data + i));
    }

    if (network->buffer_incoming_.size() == network->data_incoming_len_) {
        network->notify(network->last_received_topic_, nlohmann::json::parse(network->buffer_incoming_));
        network->last_received_topic_.clear();
        network->buffer_incoming_.clear();
        network->data_incoming_len_ = 0;
    }
}
