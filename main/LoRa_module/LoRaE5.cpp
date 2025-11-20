//
// Created by Julija Ivaske on 20.11.2025.
//

#include "LoRaE5.h"

static const char* TAG = "LoRaE5";

LoRaE5::LoRaE5(uint32_t TX_pin, uint32_t RX_pin)
    : tx_pin(TX_pin), rx_pin(RX_pin), initialized(false)
{
}

bool LoRaE5::init()
{
    // these configs are taken from idf uart exmaple
    uart_config_t uart_config = {
        .baud_rate = LORA_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

    #if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
    #endif


    ESP_ERROR_CHECK(uart_driver_install(LORA_UART_NUM, BUFFER_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(LORA_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(LORA_UART_NUM, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));


    initialized = true;
    clear_buffer();

    return true;
}

void LoRaE5::clear_buffer()
{
    uart_flush(LORA_UART_NUM);
}

void LoRaE5::send_data(const char* data)
{
    int len = strlen(data);
    uart_write_bytes(LORA_UART_NUM, data, len);
    uart_write_bytes(LORA_UART_NUM, "\r\n", 2);
    ESP_LOGD(TAG, "Sent: %s", data);
}

int LoRaE5::receive_data(char* buffer, size_t buffer_size, uint32_t timeout)
{
    int len = uart_read_bytes(LORA_UART_NUM, (uint8_t*)buffer, buffer_size - 1, timeout / portTICK_PERIOD_MS);

    if (len > 0) {
        buffer[len] = '\0'; 
        ESP_LOGD(TAG, "Received: %s", buffer);
    } else if (len < 0) {
        ESP_LOGE(TAG, "UART read error");
        buffer[0] = '\0';
        len = 0;
    } else {
        buffer[0] = '\0';
    }

    return len;
}

bool LoRaE5::send_AT(const char* command, char* response, size_t response_size, uint32_t timeout_ms)
{
    clear_buffer();
    send_data(command);
    int len = receive_data(response, response_size, timeout_ms);
    return len > 0;
}

bool LoRaE5::wait_response(const char* expected, uint32_t timeout_ms)
{
    char buffer[BUFFER_SIZE];
    TickType_t start_time = xTaskGetTickCount();
    TickType_t timeout = pdMS_TO_TICKS(timeout_ms);

    while ((xTaskGetTickCount() - start_time) < timeout) {
        int len = receive_data(buffer, BUFFER_SIZE, 100);
        if (len > 0 && strstr(buffer, expected) != nullptr) {
            ESP_LOGI(TAG, "Expected response received: %s", expected);
            return true;
        }
    }
    ESP_LOGW(TAG, "Timeout waiting for response: %s", expected);
    return false;
}

void LoRaE5::low_power()
{
    send_data("AT+LOWPOWER=AUTOON");
    wait_response("+LOWPOWER: SLEEP");
}