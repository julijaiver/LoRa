//
// Created by Julija Ivaske on 20.11.2025.
//

#ifndef LORA_LORAE5_H
#define LORA_LORAE5_H

#include <stdint.h>
#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include <string>
#include <vector>

#include "structs.h"

//definitions for lora customizable
#define LORA_BAUDRATE 9600
#define LORA_TIMEOUT_MS 2000
#define BUFFER_SIZE 256
#define LORA_UART_NUM UART_NUM_1
#define RESPONSE_TIMEOUT_MS 15000

class LoRaE5
{
    public:
        LoRaE5(uint32_t TX_pin, uint32_t RX_pin);

        bool lora_init();
        void send_command(const char *cmd);
        void send_autoon_cmd(const char *cmd);
        void enable_lowpower(void);
        int uart_response(uint8_t *buf, int buf_size);
        int strip_autoon_prefix(uint8_t *response, int response_len, uint8_t **output_data);
        std::string read_response_with_timeout(uint32_t timeout_ms, bool strip_prefix);
        bool get_devui(std::string &devEui);
        void set_appkey(const char *appkey);
        bool initial_setup(void);
        bool join_gateway(void);

        // funcs for sending sensor data
        std::vector<uint8_t> sensor_data_payload(const sensor_data &data);
        std::string bytes_to_hex_string(const std::vector<uint8_t> &data);
        // a template to convert data to bytes and add it to the vector
        template<typename T>
        void append_bytes(std::vector<uint8_t> &vec, const T &value);
        bool send_sensor_data(const sensor_data &data);

    private:
        uint32_t tx_pin;
        uint32_t rx_pin;
        bool initialized;

        // clearing UART buffer
        // idk yet if needed
        void clear_buffer();
};


#endif //LORA_LORAE5_H