//
// Created by Julija Ivaske on 20.11.2025.
//

#include "LoRa_Task.h"
#include "esp_log.h"

//#define TX 43
//#define RX 44
#define TX 17
#define RX 16

//#define GATEWAY_SETUP_DONE
//#define UNIFIED_DATA
//#define SEPARATE_DATA

static const char* TAG = "LoRa_Controller";

LoRa_Task::LoRa_Task(QueueHandle_t to_LoRa, uint32_t stack_size, UBaseType_t priority)
            : lora(TX, RX), to_LoRa(to_LoRa)
{
    if (!lora.lora_init()) {
    ESP_LOGE(TAG, "LoRa init failed!");
    return;
    } 
    ESP_LOGI(TAG, "LoRa initialized");
    
    xTaskCreate(taskWrapper, "LORA", stack_size, this, priority, &control_handle);
}

void LoRa_Task::taskWrapper(void* pvParameters)
{
    auto *controller = static_cast<LoRa_Task*>(pvParameters);
    controller->taskImpl();
}

void LoRa_Task::taskImpl()
{
    while (true) {
        printf("Set up gateway");
        #ifdef GATEWAY_SETUP_DONE
        #ifdef UNIFIED_DATA
        sensor_data_unified data;
        if (xQueueReceive(to_LoRa, &data, portMAX_DELAY) == pdTRUE) {
            lora.send_sensor_data_unified(data);
        }
        #elif defined SEPARATE_DATA
        // test for sending sensor data
        sensor_data data;
        if (xQueueReceive(to_LoRa, &data, portMAX_DELAY) == pdTRUE) {
            lora.send_sensor_data(data);
        }
        #endif
        //wait 1s
        vTaskDelay(pdMS_TO_TICKS(1000));
        #endif
    }
}
