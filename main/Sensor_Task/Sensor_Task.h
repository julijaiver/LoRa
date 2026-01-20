#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <stdint.h>               
#include <memory>
#include "freertos/FreeRTOS.h"

class Sensor_Task
{
    public:
        Sensor_Task(QueueHandle_t to_LoRa, uint32_t stack_size = 1024, UBaseType_t priority = tskIDLE_PRIORITY + 1, TickType_t period = pdMS_TO_TICKS(30000));
        static void taskWrapper(void* pvParameters);

    private:
        void taskImpl();
        
        TaskHandle_t sensor_handle;

        QueueHandle_t to_LoRa;
        TickType_t period;
};


#endif //SENSOR_TASK_H