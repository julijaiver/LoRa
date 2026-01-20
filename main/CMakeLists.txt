idf_component_register(SRCS "main.cpp"
                            "LoRa_module/LoRaE5.cpp"
                            "Controller/LoRa_Task.cpp"
                            "Sensor_Task/Sensor_Task.cpp"
                    REQUIRES esp_driver_uart esp_driver_gpio
                    INCLUDE_DIRS "."
                                "LoRa_module"
                                "Controller"
                                "Sensor_Task"
                    )
