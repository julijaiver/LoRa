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
    sensor_data sps30_data;
    sps30_data.type = SPS30;
    sps30_data.data.sps_data.pm25_numerical = 12.34;
    sps30_data.data.sps_data.pm25_mass = 1.23;
    sps30_data.data.sps_data.pm10_numerical = 56.78;
    sps30_data.data.sps_data.pm10_mass = 4.56;

    sensor_data bmv_data;
    bmv_data.type = BMV080;
    bmv_data.data.bmv_data.pm25_numerical = 30.34;
    bmv_data.data.bmv_data.pm25_mass = 3.33;
    bmv_data.data.bmv_data.pm10_numerical = 87.78;
    bmv_data.data.bmv_data.pm10_mass = 9.22;

    sensor_data bme_data;
    bme_data.type = BME690;
    bme_data.data.bme_data.voc = 345.67;
    bme_data.data.bme_data.pressure = 1013.25;
    bme_data.data.bme_data.humidity = 45.6;

    sensor_data temp_data;
    temp_data.type = TC74A2;
    temp_data.data.t_data.temperature = 22.5;

    while (true) {
        // sensor reading and sending to LoRa task via queue would be here
        xQueueSendToBack(to_LoRa, &sps30_data, portMAX_DELAY);
        xQueueSendToBack(to_LoRa, &bmv_data, portMAX_DELAY);
        xQueueSendToBack(to_LoRa, &bme_data, portMAX_DELAY);
        xQueueSendToBack(to_LoRa, &temp_data, portMAX_DELAY);

        // incrementing test data for next send for testing purposes
        sps30_data.data.sps_data.pm25_numerical += 1.0;
        sps30_data.data.sps_data.pm10_numerical += 1.0;
        sps30_data.data.sps_data.pm25_mass += 1.0;
        sps30_data.data.sps_data.pm10_mass += 1.0;

        bmv_data.data.bmv_data.pm25_numerical += 1.0;
        bmv_data.data.bmv_data.pm10_numerical += 1.0;
        bmv_data.data.bmv_data.pm25_mass += 1.0;
        bmv_data.data.bmv_data.pm10_mass += 1.0;
        
        bme_data.data.bme_data.voc += 1.0;
        bme_data.data.bme_data.pressure += 1.0;
        bme_data.data.bme_data.humidity += 1.0;
        
        temp_data.data.t_data.temperature += 1.0;


        vTaskDelayUntil(&last_wake_time, period);
    }
}