#pragma once

#include <string>

namespace NetworkConfig {

//Wi-fi config
const std::string SSID {""};
const std::string PASS {""};

//MQTT Config
const std::string CLIENT_USER {""};
const std::string CLIENT_PASS {""};

std::string MQTT_SERVER_HOST {""};
constexpr uint16_t MQTT_SERVER_PORT {8883};

std::string CA_CERTIFICATE {"-----BEGIN CERTIFICATE-----\n\
-----END CERTIFICATE-----\n"};
}
