#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "Controller/LoRa_Task.h"
#include "Sensor_Task/Sensor_Task.h"


extern "C" void app_main(void)
{
    QueueHandle_t to_LoRa = xQueueCreate(10, sizeof(sensor_data));
    static LoRa_Task controller(to_LoRa);
    static Sensor_Task sensor(to_LoRa);

}
