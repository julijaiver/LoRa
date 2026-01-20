//
// Created by Julija Ivaske on 20.11.2025.
//

#ifndef CONTROLLER_CONTROLLER_H
#define CONTROLLER_CONTROLLER_H

#include <stdint.h>               
#include "freertos/FreeRTOS.h"    
#include "freertos/task.h"        
#include "LoRa_module/LoRaE5.h"

class LoRa_Task
{
    public:
        LoRa_Task(QueueHandle_t to_LoRa, uint32_t stack_size = 4096, UBaseType_t priority = tskIDLE_PRIORITY + 1);
        static void taskWrapper(void* pvParameters);

    private:
        void taskImpl();
        //bool initialize_lora(); // probably not needed now, implemented it differently
        //void send_AT(const char* command);
        //void parse_AT_response(const char* response);

        TaskHandle_t control_handle;
        LoRaE5 lora;

        QueueHandle_t to_LoRa;
        // var for saving AT response
        const char* at_response;
};


#endif //CONTROLLER_CONTROLLER_H