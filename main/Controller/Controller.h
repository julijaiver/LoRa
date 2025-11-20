//
// Created by Julija Ivaske on 20.11.2025.
//

#ifndef CONTROLLER_CONTROLLER_H
#define CONTROLLER_CONTROLLER_H

#include <stdint.h>               
#include "freertos/FreeRTOS.h"    
#include "freertos/task.h"        
#include "LoRa_module/LoRaE5.h"

class Controller
{
    public:
        Controller(uint32_t stack_size = 1024, UBaseType_t priority = tskIDLE_PRIORITY + 1);
        static void taskWrapper(void* pvParameters);

    private:
        void taskImpl();
        bool initialize_lora();
        //void send_AT(const char* command);
        //void parse_AT_response(const char* response);

        TaskHandle_t control_handle;
        const char *name = "CONTROLLER";
        // lora object, maybe better pointer?
        LoRaE5 lora;
        // var for saving AT response
        const char* at_response;
};


#endif //CONTROLLER_CONTROLLER_H