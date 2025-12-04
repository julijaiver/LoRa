//
// Created by Julija Ivaske on 20.11.2025.
//

#include "Controller.h"

static const char* TAG = "Controller";

Controller::Controller(uint32_t stack_size, UBaseType_t priority): lora(17, 18)
{
    if (!lora.lora_init()) {
    ESP_LOGE(TAG, "LoRa init failed!");
    return;
    } 
    ESP_LOGI(TAG, "LoRa initialized");
    
    // need to handle retries probably if init fails 

    xTaskCreate(taskWrapper, "CONTROLLER", stack_size, this, priority, &control_handle);
}

void Controller::taskWrapper(void* pvParameters)
{
    auto *controller = static_cast<Controller*>(pvParameters);
    controller->taskImpl();
}

void Controller::taskImpl()
{
    //lora.lora_init();
    //ESP_LOGI(TAG, "LoRa initialized successfully");

    vTaskDelay(pdMS_TO_TICKS(200));

    // getting devEui 
    char devEui[32];
    if (lora.get_devui(devEui, sizeof(devEui))) {
        ESP_LOGI(TAG, "Device EUI: %s", devEui);
    } else {
        ESP_LOGE(TAG, "Failed to get Device EUI");
    }

    while (true) {
        // now just sending AT command every 10s 
        lora.send_autoon_cmd("AT");
        lora.read_autoon_response();
        vTaskDelay(pdMS_TO_TICKS(10000)); 
    }
}