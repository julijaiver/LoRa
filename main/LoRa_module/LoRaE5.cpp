//
// Created by Julija Ivaske on 20.11.2025.
//


// TODO: implement send function
// TODO: implement error handling and retries


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
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(LORA_UART_NUM, BUFFER_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(LORA_UART_NUM, &uart_config);
    uart_set_pin(LORA_UART_NUM, tx_pin, rx_pin,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG, "UART ready");

    // init commands to enable low power mode, set lwotaa mode and join network
    vTaskDelay(pdMS_TO_TICKS(100));

    // things to do just once at the beginning of using the module
    /*
    send_autoon_cmd("AT+MODE=LWOTAA");
    read_response_with_timeout(RESPONSE_TIMEOUT_MS, true);
    std::string devEui;
    get_devui(devEui);
    ESP_LOGI(TAG, "Device EUI: %s", devEui.c_str());
    send_autoon_cmd("AT+KEY=APPKEY, 8e04c3ff3d92666cf0a92de2a93f8962"); 
    read_response_with_timeout(RESPONSE_TIMEOUT_MS, true);

    send_autoon_cmd("AT+KEY=APPKEY");
    std::string appkey_verify = read_response_with_timeout(RESPONSE_TIMEOUT_MS, true);
    ESP_LOGI(TAG, "AppKey verify: %s", appkey_verify.c_str());
    */


    if (!initial_setup()) {
        ESP_LOGE(TAG, "LoRa module initial setup failed");
        return false;
    }

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
/*int LoRaE5::uart_response(uint8_t *buf, int buf_size) {
    int len = uart_read_bytes(LORA_UART_NUM, buf, buf_size, pdMS_TO_TICKS(500));

    if (len <= 0) {
        ESP_LOGW(TAG, "No response");
        return 0;
    } 
    buf[len] = '\0';
    return len;
}*/

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

std::string LoRaE5::read_response_with_timeout(uint32_t timeout_ms, bool strip_prefix) {
    std::string response;
    uint8_t buffer[BUFFER_SIZE];
    TickType_t start_tick = xTaskGetTickCount();

    while ((xTaskGetTickCount() - start_tick) < pdMS_TO_TICKS(timeout_ms)) {
        TickType_t elapsed = xTaskGetTickCount() - start_tick;
        TickType_t remaining = pdMS_TO_TICKS(timeout_ms) - elapsed;

        if (remaining <= 0) {
            break; // timeout reached
        }

        int len = uart_read_bytes(LORA_UART_NUM, buffer, BUFFER_SIZE-1, remaining);

        if (len > 0) {
            uint8_t *data_ptr = buffer;
            int data_len = len;

            if (strip_prefix) {
                data_len = strip_autoon_prefix(buffer, len, &data_ptr);
            }

            response.append(reinterpret_cast<char*>(data_ptr), data_len);
            vTaskDelay(pdMS_TO_TICKS(10)); 
        } else {
            if (!response.empty()) {
                break; 
            }
            vTaskDelay(pdMS_TO_TICKS(50)); // wait before retrying
        }
    }
    return response;
}

// devEui is passed as array and filled in the func. This as I understand is only used once
// to get the ID of the module for registering in the LoRa network and after that only need AT+JOIN
bool LoRaE5::get_devui(std::string &devEui) {
    send_autoon_cmd("AT+ID=DevEui");

    std::string response = read_response_with_timeout(RESPONSE_TIMEOUT_MS, true); // 15s timeout for reading
    if (response.empty()) {
        ESP_LOGE(TAG, "No response for DevEui request");
        return false;
    }

    ESP_LOGI(TAG, "Raw response: %s", response.c_str());

    // expected response from datasheet: +ID: DevEui, xx:xx:xx:xx:xx:xx:xx:xx
    // reading after comma and also removing colons (do we need id without them?)
    size_t pos = response.find("+ID:");
    if (pos == std::string::npos) {
        return false;
    }

    pos = response.find(',', pos);
    if (pos == std::string::npos) {
        return false;
    }

    ++pos; // move past comma
    while (response[pos] == ' ') {
        ++pos; // skip spaces
    }

    // get hex without colons
    std::string devEuiHex;
    while (pos < response.size() && response[pos] != '\r' && response[pos] != '\n') {
        if (response[pos] != ':') {
            devEuiHex.push_back(response[pos]);
        }
        ++pos;
    }
    devEui = devEuiHex;
    return true;
}

// this maybe need to do with the gateway :D
bool LoRaE5::join_gateway(void) {
    send_autoon_cmd("AT+JOIN");

    std::string response = read_response_with_timeout(RESPONSE_TIMEOUT_MS, true);
    if (response.empty()) {
        ESP_LOGE(TAG, "No response for JOIN command");
        return false;
    }
    ESP_LOGI(TAG, "JOIN response: %s", response.c_str());

    if (response.find("+JOIN: Done") != std::string::npos) {
        return true;
    } else {
        return false;
    }
}

bool LoRaE5::initial_setup(void) {
    // assuming that devEui and appkey are already set (also LWOTAA mode) (need to set just once), 
    // this is a switch statement to join network when device turned on
    setup_states setup = SET_AUTOON;
    
    while (setup != SETUP_DONE) {
        switch (setup) {
            case SET_AUTOON:
                enable_lowpower();
                read_response_with_timeout(RESPONSE_TIMEOUT_MS, false);
                setup = JOIN_NETWORK;
                break;
            case JOIN_NETWORK: {
                int retry = 0;
                bool joined = false;

                while (retry < 3) {
                    if (join_gateway()) {
                        ESP_LOGI(TAG, "Joined LoRa network successfully");
                        joined = true;
                        break;
                    } 
                    ++retry;
                    if (retry < 3) {
                        ESP_LOGW(TAG, "Retrying to join LoRa network (%d/3)", retry);
                        vTaskDelay(pdMS_TO_TICKS(3000));
                    }
                }
                if (joined) {
                    setup = SETUP_DONE;
                } else {
                    ESP_LOGE(TAG, "Failed to join LoRa network after %d retries", retry);
                    return false;
                }
                break;
            }
            case SETUP_DONE:
                break;
        }
    }
    ESP_LOGI(TAG, "LoRa module setup done, ready to send");
    return true;
}
