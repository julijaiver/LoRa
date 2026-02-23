#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
//#include "Controller/LoRa_Task.h"
//#include "Sensor_Task/Sensor_Task.h"


extern "C" void app_main(void)
{
    //QueueHandle_t to_LoRa = xQueueCreate(10, sizeof(sensor_data));
    //static LoRa_Task controller(to_LoRa);
    //static Sensor_Task sensor(to_LoRa);
    
   uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(UART_NUM_1, 512, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    while (true) {
        uint8_t buf[128] = {0};
        int len = uart_read_bytes(UART_NUM_1, buf, sizeof(buf)-1, pdMS_TO_TICKS(1000));
        if (len > 0) {
            buf[len] = 0;
            ESP_LOGI("TEST", "Got: %s", buf);
        }
    }
}
