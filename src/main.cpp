#include "build/_deps/json-src/include/nlohmann/json.hpp"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "src/Network/ErrorCode.hpp"
#include "src/Network/Network.hpp"
#include "src/Network/Receiver.hpp"
#include <iostream>

enum PIN {
    SDA = 6,
    SCL,
};

constexpr uint8_t sensor_addr {0x10};

uint16_t check_light() {
    constexpr uint8_t reading_reg {0x04};
    std::array<uint8_t, 2> buffer {0, 0};
    i2c_write_blocking(i2c1, sensor_addr, &reading_reg, 1, true);
    i2c_read_blocking(i2c1, sensor_addr, buffer.data(), 2, false);
    return (buffer[1] << 8) | buffer[0];
}

int main() {
    stdio_init_all();

    i2c_init(i2c1, 100'000);
    gpio_set_function(PIN::SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN::SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN::SDA);
    gpio_pull_up(PIN::SCL);

    Network network;
    if (network.init() != ErrorCode::OK) {
        std::cout << "Network not initialized\n";
        return 0;
    }

    Receiver rx;
    rx.set_notifier(&network);
    rx.subscribe("pico_w/recv/");
    rx.subscribe("pico_w/recv/");
    rx.subscribe("pico_w/recv/");
    rx.subscribe("pico_w/temp/");
    rx.unsubscribe("pico_w/recv/");
    rx.unsubscribe("pico_w/recv/");

    std::array<uint8_t, 2> als_conf_0 {0x00, 0x00};
    i2c_write_blocking(i2c1, sensor_addr, als_conf_0.data(), als_conf_0.size(), false);

    nlohmann::json data;

    while (true) {
        network.loop();
        data["light"] = check_light();
        network.publish(data.dump(), "pico_w/test/", 0, 0);
        sleep_ms(1000);
    }
}
