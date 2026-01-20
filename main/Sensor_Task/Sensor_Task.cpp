#include "Sensor_Task.h"
#include "structs.h"

Sensor_Task::Sensor_Task(QueueHandle_t to_LoRa, uint32_t stack_size, UBaseType_t priority, TickType_t period)
            : to_LoRa(to_LoRa), period(period)
{
    xTaskCreate(taskWrapper, "SENSOR", stack_size, this, priority, &sensor_handle);
}

void Sensor_Task::taskWrapper(void* pvParameters)
{
    auto *sensor_task = static_cast<Sensor_Task*>(pvParameters);
    sensor_task->taskImpl();
}

void Sensor_Task::taskImpl()
{
    TickType_t last_wake_time = xTaskGetTickCount();
    
    // adding test data to structs
    sensor_data ppm_data;
    ppm_data.type = PARTICULATE;
    ppm_data.data.p_data.pm25 = 12.34;
    ppm_data.data.p_data.pm10 = 56.78;

    sensor_data bme_data;
    bme_data.type = BME690;
    bme_data.data.b_data.voc = 345.67;
    bme_data.data.b_data.pressure = 1013.25;
    bme_data.data.b_data.humidity = 45.6;

    sensor_data temp_data;
    temp_data.type = TEMPERATURE;
    temp_data.data.t_data.temperature = 22.5;

    while (true) {
        // sensor reading and sending to LoRa task via queue would be here
        xQueueSendToBack(to_LoRa, &ppm_data, portMAX_DELAY);
        xQueueSendToBack(to_LoRa, &bme_data, portMAX_DELAY);
        xQueueSendToBack(to_LoRa, &temp_data, portMAX_DELAY);

        // incrementing test data for next send
        ppm_data.data.p_data.pm25 += 1.0;
        ppm_data.data.p_data.pm10 += 1.0;
        
        bme_data.data.b_data.voc += 1.0;
        bme_data.data.b_data.pressure += 1.0;
        bme_data.data.b_data.humidity += 1.0;
        
        temp_data.data.t_data.temperature += 1.0;


        vTaskDelayUntil(&last_wake_time, period);
    }
}