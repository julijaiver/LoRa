//
// Created by Julija Ivaske on 20.11.2025.
//

#include <sstream>
#include <iomanip>
#include <cstring>
#include <map>

#include "LoRaE5.h"

#define APPKEY "8e04c3ff3d92666cf0a92de2a93f8962"

static const char* TAG = "LoRaE5";

static const std::map<int8_t, const char*> lora_errors {
    { -1, "Parameter is invalid" },
    { -10, "Command is unknown" },
    { -11, "Command is in wrong format" },
    { -12, "Command is unavailable in current mode (Check with \"AT+MODE\")"},
    { -20, "Too many parameters. LoRaWAN modem support max 15 parameters" },
    { -21, "Length of command is too long (exceed 528 bytes)" },
    { -22, "Receive end symbol timeout, command must end with <LF>" },
    { -23, "Invalid character received." },
    { -24, "Either -21, -22 or -23." }
};

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
    send_autoon_cmd("AT+MODE=LWOTAA");
    std::string response {};
    response = read_response_with_timeout(RESPONSE_TIMEOUT_MS, true);
    if (response != "+MODE: LWOTAA\r\n") {
        ESP_LOGE(TAG, "Failed to enter LWOTAA mode. Got response: '%s'", response.c_str());
        return false;
    }
    std::string devEui;
    get_devui(devEui);
    ESP_LOGI(TAG, "Device EUI: %s", devEui.c_str());
    send_autoon_cmd("AT+KEY=APPKEY, " APPKEY); 
    read_response_with_timeout(RESPONSE_TIMEOUT_MS, true);
    
    if (!initial_setup()) {
        ESP_LOGE(TAG, "LoRa module initial setup failed");
        uart_driver_delete(LORA_UART_NUM);
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
    uart_flush(LORA_UART_NUM);

    ESP_LOGI(TAG, "<< %s", cmd);
}

// again simple func to add 4x 0xFF prefix to a command when sending in autoon mode 
void LoRaE5::send_autoon_cmd(const char *cmd)
{
    uint8_t wake_prefix[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    uart_write_bytes(LORA_UART_NUM, (const char *)wake_prefix, 4);
    send_command(cmd);
    //ESP_LOGI(TAG, ">> [AUTOON] %s", cmd);
}

// used in the beginning in init
void LoRaE5::enable_lowpower(void) {
    send_command("AT+LOWPOWER=AUTOON");
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

// this func is for simple responses like getting devEui
std::string LoRaE5::read_response_with_timeout(uint32_t timeout_ms, bool strip_prefix) {
    std::string response { "" };
    uint8_t buffer[BUFFER_SIZE] { 0 };
    TickType_t start_tick = xTaskGetTickCount();
    TickType_t last_data_tick = start_tick;
    bool first_read = true;

    while (true) {
        TickType_t elapsed = xTaskGetTickCount() - start_tick;
        // Must be a signed int to avoid underflow

        if (elapsed >= pdMS_TO_TICKS(timeout_ms)) {
            //ESP_LOGI(TAG, "read_response_with_timeout timed out.");
            break; // timeout reached
        }

        int remaining = pdMS_TO_TICKS(timeout_ms) - elapsed;
        int len = uart_read_bytes(LORA_UART_NUM, buffer, BUFFER_SIZE-1, remaining < pdMS_TO_TICKS(100) ? remaining : pdMS_TO_TICKS(100));

        if (len > 0) {
            uint8_t *data_ptr = buffer;
            int data_len = len;

            if (strip_prefix && first_read) {
                data_len = strip_autoon_prefix(buffer, len, &data_ptr);
                first_read = false;
            }

            response.append(reinterpret_cast<char*>(data_ptr), data_len);
            last_data_tick = xTaskGetTickCount();
        } else if (!response.empty()) {
            if (xTaskGetTickCount() - last_data_tick >= pdMS_TO_TICKS(100)) {
                // no new data for 100ms after receiving some data
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    /*
       Can we do something else with errors other than log them?
       Maybe talk with deployment team about sending them to the terminal
       used during deployment?
    */
    log_error_response(response);
    return response;
}

// this func to use when there are multiple responses and waiting for specific ones
std::string LoRaE5::read_until_found(const std::vector<std::string> &expected_responses, uint32_t timeout_ms, bool strip_prefix) {
    std::string response { "" };
    uint8_t buffer[BUFFER_SIZE] { 0 };
    TickType_t start_tick = xTaskGetTickCount();
    bool first_read = true;

    while(true) {
        TickType_t elapsed = xTaskGetTickCount() - start_tick;
        if (elapsed >= pdMS_TO_TICKS(timeout_ms)) {
            ESP_LOGE(TAG, "Response timeout");
            return ""; // timeout reached, return empty string
        }

        int remaining = pdMS_TO_TICKS(timeout_ms) - elapsed;
        int len = uart_read_bytes(LORA_UART_NUM, buffer, BUFFER_SIZE - 1, remaining < 200 ? remaining : 200); // wait max 200ms per read
        if (len > 0) {
            uint8_t *data_ptr = buffer;
            int data_len = len;

            if (strip_prefix && first_read) { // to only strip prefix from first read
                data_len = strip_autoon_prefix(buffer, len, &data_ptr);
                first_read = false;
            }

            response.append(reinterpret_cast<char*>(data_ptr), data_len);

            log_error_response(response);
            // check if any expected response is found 
            for (const auto &expected : expected_responses) {
                if (response.find(expected) != std::string::npos) {
                    return response; 
                }
            }
        } 
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

void LoRaE5::log_error_response(const std::string &response) {
    std::size_t error_pos { response.find("ERROR(") };
    if (error_pos != std::string::npos) {
        constexpr std::size_t skip { std::string_view{"ERROR("}.length() };
        error_pos += skip;
        const std::size_t error_end { response.find(")", error_pos) };
        int error_code {};
        const std::errc ec { std::from_chars(response.data() + error_pos, response.data() + error_end, error_code).ec };
        if (ec == std::errc::invalid_argument || ec == std::errc::result_out_of_range) {
            ESP_LOGE(TAG, "Failed to parse LoRa error code. Error: '%s'", response.c_str());
        } else {
            if (lora_errors.contains(error_code)) {
                ESP_LOGE(TAG, "Got error from lora module: %d: %s", error_code, lora_errors.at(error_code));
            } else {
                ESP_LOGE(TAG, "Got unknown error from lora module: %d", error_code);
            }
        }
    }
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

bool LoRaE5::join_gateway(void) {
    send_autoon_cmd("AT+JOIN");
    std::string response = read_until_found({"+JOIN: Done", "+JOIN: Join failed"}, JOIN_TIMEOUT_MS, true);

    bool joined = response.find("+JOIN: Done") != std::string::npos;
    if (!joined) {
        ESP_LOGE(TAG, "Failed to join LoRa network. Response: %s", response.c_str());
    }
    return joined;
}

bool LoRaE5::initial_setup(void) {
    // assuming that devEui and appkey are already set (also LWOTAA mode) (need to set just once), 
    // this is a switch statement to join network when device turned on
    enable_lowpower();
    read_response_with_timeout(RESPONSE_TIMEOUT_MS, false);

    for (int attempt = 1; attempt <= 3; ++attempt) {
        ESP_LOGI(TAG, "Joining LoRa network, attempt %d", attempt);
        if (join_gateway()) {
            ESP_LOGI(TAG, "Successfully joined LoRa network");
            return true;
        }

        if (attempt < 3) {
            ESP_LOGI(TAG, "Retrying to join LoRa network in 5 seconds...");
            vTaskDelay(pdMS_TO_TICKS(5000));
        } 
    }
    ESP_LOGE(TAG, "Failed to join LoRa network on attempt 3");
    return false;
}


// template so it can be used for different data types
template<typename T>
void LoRaE5::append_bytes(std::vector<uint8_t> &vector, const T &value) {
    const uint8_t *byte_ptr = reinterpret_cast<const uint8_t*>(&value);
    vector.insert(vector.end(), byte_ptr, byte_ptr + sizeof(T));
}

// appending sensor data to payload vector
std::vector<uint8_t> LoRaE5::sensor_data_payload(const sensor_data &data) {
    std::vector<uint8_t> payload;

    payload.push_back(static_cast<uint8_t>(data.type)); // first byte is data type

    switch (data.type) {
        case SPS30:
            append_bytes(payload, data.data.sps_data.pm25_numerical);
            append_bytes(payload, data.data.sps_data.pm25_mass);
            append_bytes(payload, data.data.sps_data.pm10_numerical);
            append_bytes(payload, data.data.sps_data.pm10_mass);
            break;
        case BMV080:
            append_bytes(payload, data.data.bmv_data.pm25_numerical);
            append_bytes(payload, data.data.bmv_data.pm25_mass);
            append_bytes(payload, data.data.bmv_data.pm10_numerical);
            append_bytes(payload, data.data.bmv_data.pm10_mass);
            break;
        case BME690:
            append_bytes(payload, data.data.bme_data.voc);
            append_bytes(payload, data.data.bme_data.pressure);
            append_bytes(payload, data.data.bme_data.humidity);
            break;
        case TC74A2:
            append_bytes(payload, data.data.t_data.temperature);
            break;
        default:
            ESP_LOGW(TAG, "Unknown sensor data type");
            break;
    }
    return payload;
}

std::string LoRaE5::bytes_to_hex_string(const std::vector<uint8_t> &data) {
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    for (uint8_t byte : data) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

bool LoRaE5::send_sensor_data(const sensor_data &data) {
    auto payload = sensor_data_payload(data);
    // converting payload to hex string for sending via AT command
    std::string hex_payload = bytes_to_hex_string(payload);
    std::string at_command = "AT+MSGHEX=\"" + hex_payload + "\"";

    send_autoon_cmd(at_command.c_str());

    std::string response = read_until_found({"+MSGHEX: Done", "+MSGHEX: Please join network first", "ERROR"}, RESPONSE_TIMEOUT_MS, true);

    bool sent = response.find("+MSGHEX: Done") != std::string::npos;
    if (sent) {
        ESP_LOGI(TAG, "Sensor data sent successfully");
    } else {
        ESP_LOGE(TAG, "Failed to send sensor data. Response: %s", response.c_str());
    }
    return sent;
}
