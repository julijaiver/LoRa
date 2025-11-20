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

//definitions for lora customizable
#define LORA_BAUDRATE 9600
#define LORA_TIMEOUT_MS 2000
#define BUFFER_SIZE 256
#define LORA_UART_NUM UART_NUM_1

class LoRaE5
{
    public:
        LoRaE5(uint32_t TX_pin, uint32_t RX_pin);

        bool init();
        void send_data(const char* data);
        int receive_data(char* buffer, size_t buffer_size, uint32_t timeout_ms);
        bool send_AT(const char* command, char* response, size_t response_size, uint32_t timeout_ms);
        bool wait_response(const char* expected, uint32_t timeout=LORA_TIMEOUT_MS);
        //int receiveData(char* buffer, size_t buffer_size, uint32_t timeout=LORA_TIMEOUT_MS);
        void low_power();
    private:
        uint32_t tx_pin;
        uint32_t rx_pin;
        bool initialized;

        // clearing UART buffer
        void clear_buffer();
};


#endif //LORA_LORAE5_H