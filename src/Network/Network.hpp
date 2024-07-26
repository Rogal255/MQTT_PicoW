#pragma once

#include "build/_deps/json-src/include/nlohmann/json.hpp"
#include "lwip/altcp_tls.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "pico/cyw43_arch.h"
#include "src/INotifier.hpp"
#include "src/Network/ErrorCode.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace NetworkHelpers {
static void mqtt_pub_start_cb(void* arg, const char* topic, u32_t tot_len);
static void mqtt_pub_data_cb(void* arg, const u8_t* data, u16_t len, u8_t flags);
}

enum class Subscription { UNSUBSCRIBE, SUBSCRIBE };

class Network : public INotifier {
public:
    ~Network() override = default;
    ErrorCode attach(IListener* listener, const std::string& topic) noexcept override;
    ErrorCode detach(IListener*, const std::string&) noexcept override;
    ErrorCode notify(const std::string& topic, const nlohmann::json& data) noexcept override;

    ErrorCode init();
    ErrorCode publish(const std::string& buffer, const std::string& topic, const uint8_t qos, const uint8_t retain);
    void loop();

private:
    const std::string client_id_ {"PicoW"};

    struct MQTT_CLIENT_T {
        ip_addr_t remote_addr;
        mqtt_client_t* mqtt_client;
    };

    std::unique_ptr<MQTT_CLIENT_T> state_ {nullptr};

    u32_t data_incoming_len_ = 0;
    std::string last_received_topic_;
    std::string buffer_incoming_;

    ErrorCode wifi_connect();
    ErrorCode mqtt_client_init();
    ErrorCode dns_lookup();
    ErrorCode broker_connect();
    ErrorCode subscribe(const std::string& topic, const Subscription sub, const uint8_t qos);

    std::vector<std::pair<IListener*, std::string>> observers_;

public:
    friend void NetworkHelpers::mqtt_pub_start_cb(void* arg, const char* topic, u32_t tot_len);
    friend void NetworkHelpers::mqtt_pub_data_cb(void* arg, const u8_t* data, u16_t len, u8_t flags);
};
