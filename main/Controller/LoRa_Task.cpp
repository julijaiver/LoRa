//
// Created by Julija Ivaske on 20.11.2025.
//

#include "LoRa_Task.h"
#include "esp_log.h"

static const char* TAG = "LoRa_Controller";

LoRa_Task::LoRa_Task(QueueHandle_t to_LoRa, uint32_t stack_size, UBaseType_t priority)
            : lora(17, 18), to_LoRa(to_LoRa)
{
    if (!lora.lora_init()) {
    ESP_LOGE(TAG, "LoRa init failed!");
    return;
    } 
    ESP_LOGI(TAG, "LoRa initialized");
    
    // need to handle retries probably if init fails 

    xTaskCreate(taskWrapper, "LORA", stack_size, this, priority, &control_handle);
}

void LoRa_Task::taskWrapper(void* pvParameters)
{
    auto *controller = static_cast<LoRa_Task*>(pvParameters);
    controller->taskImpl();
}

void LoRa_Task::taskImpl()
{
    //lora.lora_init();
    //ESP_LOGI(TAG, "LoRa initialized successfully");

    //vTaskDelay(pdMS_TO_TICKS(200));

    /*// getting devEui 
    char devEui[32];
    if (lora.get_devui(devEui, sizeof(devEui))) {
        ESP_LOGI(TAG, "Device EUI: %s", devEui);
    } else {
        ESP_LOGE(TAG, "Failed to get Device EUI");
    }*/

    while (true) {
        // now just sending AT command every 10s 
        /*lora.send_autoon_cmd("AT+MSG=\"Hello from LoRaE5\"");
        lora.read_response_with_timeout(2000, false);
        vTaskDelay(pdMS_TO_TICKS(10000)); */

        // test for sending sensor data
        sensor_data data;
        if (xQueueReceive(to_LoRa, &data, portMAX_DELAY) == pdTRUE) {
            lora.send_sensor_data(data);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}