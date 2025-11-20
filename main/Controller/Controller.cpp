//
// Created by Julija Ivaske on 20.11.2025.
//

#include "Controller.h"

static const char* TAG = "Controller";

Controller::Controller(uint32_t stack_size, UBaseType_t priority): lora(17, 18)
{
    lora.init();
    xTaskCreate(taskWrapper, name, stack_size, this, priority, &control_handle);
}

void Controller::taskWrapper(void* pvParameters)
{
    auto *controller = static_cast<Controller*>(pvParameters);
    controller->taskImpl();
}

bool Controller::initialize_lora()
{
    char response[BUFFER_SIZE] = {0};

    ESP_LOGI(TAG, "Initializing LoRa module");
    ESP_LOGI(TAG, "Sending AT command");

    for (int attempt = 1; attempt <= 3; ++attempt) {
        ESP_LOGI(TAG, "Attempt %d to send AT command", attempt);
        if (lora.send_AT("AT", response, BUFFER_SIZE, LORA_TIMEOUT_MS)) {
            break;
        }
        ESP_LOGW(TAG, "No response received, retrying...");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (strstr(response, "+AT: OK") == NULL) {
        ESP_LOGE(TAG, "Did not receive '+AT: OK'. Response: %s", response);
        return false;
    }
    ESP_LOGI(TAG, "✓ Received '+AT: OK'");
    vTaskDelay(pdMS_TO_TICKS(100));

    // getting version, first setting response buffer to zeroes
    memset(response, 0, BUFFER_SIZE);
    if (!lora.send_AT("AT+VER", response, BUFFER_SIZE, LORA_TIMEOUT_MS)) return false;
    if (!strstr(response, "+VER:")){
        ESP_LOGE(TAG, "Failed to get version. Response: %s", response);
        return false;
    } else {
        ESP_LOGI(TAG, "✓ LoRa Module Version: %s", response);
    }

    // putting lora to ultra low power mode
    memset(response, 0, BUFFER_SIZE);
    if (!lora.send_AT("AT+LOWPOWER=AUTOON", response, BUFFER_SIZE, LORA_TIMEOUT_MS)) return false;
    if (!strstr(response, "+LOWPOWER: SLEEP")){
        ESP_LOGE(TAG, "Failed to set low power mode. Response: %s", response);
        return false;
    } else {
        ESP_LOGI(TAG, "✓ LoRa Module set to low power mode");
    }

    return true;

}

void Controller::taskImpl()
{
    if (!initialize_lora()) {
        ESP_LOGE(TAG, "LoRa initialization failed");
        vTaskDelete(nullptr);  // maybe just need to retry several times 
        return;
    }

    ESP_LOGI(TAG, "LoRa initialized successfully");

    // main controll loop idk what to do here yet
    while (true) {
        printf("Probably waiting for event bit to get sensor readingzzz\n");
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}