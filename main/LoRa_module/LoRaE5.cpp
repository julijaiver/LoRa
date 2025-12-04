//
// Created by Julija Ivaske on 20.11.2025.
//

#include "LoRaE5.h"

static const char* TAG = "LoRaE5";

LoRaE5::LoRaE5(uint32_t TX_pin, uint32_t RX_pin)
    : tx_pin(TX_pin), rx_pin(RX_pin), initialized(false)
{
}

bool LoRaE5::lora_init()
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

    uart_driver_install(LORA_UART_NUM, BUFFER_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(LORA_UART_NUM, &uart_config);
    uart_set_pin(LORA_UART_NUM, tx_pin, rx_pin,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG, "UART ready");

    vTaskDelay(pdMS_TO_TICKS(100));
    enable_lowpower();
    read_autoon_response();

    // need to maybe improve error checking in init
    return true;
}

// just simple func for sending a command
void LoRaE5::send_command(const char *cmd)
{
    uart_write_bytes(LORA_UART_NUM, cmd, strlen(cmd));
    uart_write_bytes(LORA_UART_NUM, "\r\n", 2);

    ESP_LOGI(TAG, ">> %s", cmd);
}

// again simple func to add 4x 0xFF prefix to a command when sending in autoon mode 
void LoRaE5::send_autoon_cmd(const char *cmd)
{
    uint8_t wake_prefix[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    uart_write_bytes(LORA_UART_NUM, (const char *)wake_prefix, 4);
    send_command(cmd);

    ESP_LOGI(TAG, "<< [AUTOON] %s", cmd);
}

// used in the beginning in init
void LoRaE5::enable_lowpower(void) {
    send_command("AT+LOWPOWER=AUTOON");
}

// returns length of response from passed buffer and null terminates
// this only reads response once, so need to think how to read multiple messages back, maybe read until some timeout
int LoRaE5::uart_response(uint8_t *buf, int buf_size) {
    int len = uart_read_bytes(LORA_UART_NUM, buf, buf_size, pdMS_TO_TICKS(500));

    if (len <= 0) {
        ESP_LOGW(TAG, "No response");
        return 0;
    } 
    buf[len] = '\0';
    return len;
}

int LoRaE5::strip_autoon_prefix(uint8_t *response, int response_len, uint8_t **output_data) {
    // validate response prefix and strip it
    if (response_len >= 4 &&
        response[0] == 0xFF && response[1] == 0xFF &&
        response[2] == 0xFF && response[3] == 0xFF)
    {
        *output_data = response + 4;
        return response_len - 4;
    }
    *output_data = response;
    return response_len;
}

void LoRaE5::read_autoon_response(void)
{
    uint8_t response[BUFFER_SIZE];
    int response_len = uart_response(response, BUFFER_SIZE);

    if (response_len == 0) {
        return;
    }

    // get all bytes (for debugging)
    printf(">> RAW BYTES (%d): ", response_len);
    for (int i = 0; i < response_len; ++i) {
        printf("%02X ", response[i]);
    }
    printf("\n");

    uint8_t *data;
    int data_len = strip_autoon_prefix(response, response_len, &data);
    data[data_len] = '\0';
    ESP_LOGI(TAG, ">> %s", (char *)data);
}

// devEui is passed as array and filled in the func. This as I understand is only used once
// to get the ID of the module for registering in the LoRa network and after that only need AT+JOIN
bool LoRaE5::get_devui(char *output_data, size_t output_size) {
    send_autoon_cmd("AT+ID=DevEui");

    uint8_t response[BUFFER_SIZE];
    int response_len = uart_response(response, BUFFER_SIZE);

    if (response_len == 0) {
        return false;
    }

    uint8_t *data;
    int data_len = strip_autoon_prefix(response, response_len, &data);
    if (data_len < 1) {
        ESP_LOGE(TAG, "No data after stripping prefix");
        return false;
    }
    data[data_len] = '\0';

    ESP_LOGI(TAG, "Raw response: %s", data);

    if (!strstr((char *)data, "+ID:")) {
        ESP_LOGE(TAG, "Incorrect response format");
        return false;
    }

    // expected response from datasheet: +ID: DevEui, xx:xx:xx:xx:xx:xx:xx:xx
    // reading after comma and also removing colons (do we need id without them?)

    char *comma_pos = strchr((char *)data, ',');
    if (!comma_pos) {
        ESP_LOGE(TAG, "Unexpected response format for devEui");
        return false;
    }
    comma_pos++;  
    while (*comma_pos == ' ') comma_pos++;

    size_t j = 0;
    for (size_t i = 0; comma_pos[i] != '\0' && comma_pos[i] != '\r' && comma_pos[i] != '\n'; ++i) {
        if (comma_pos[i] != ':' && j < output_size - 1) {
            output_data[j++] = comma_pos[i];
        }
    }
    output_data[j] = '\0';
    return true;
}

// this maybe need to do with the gateway :D
void LoRaE5::join_gateway(void) {
    send_autoon_cmd("AT+JOIN");

    uint8_t response[BUFFER_SIZE];
    int response_len = uart_response(response, BUFFER_SIZE);

    if (response_len > 0) {
        // here more things will be received than just one line and need to handle possible errors
        ESP_LOGI(TAG, "Join response: %s", response);
    }
    // TODO: read all response messages not just one line
}

// TODO: implement send function
// TODO: implement error handling and retries