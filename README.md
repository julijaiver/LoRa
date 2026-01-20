LoRa-E5 module implementation for collecting sensor data and sending to a LoRaWAN gateway 

LoRa-E5 module is using UART communication and TX/RX pins are specified in LoRa_Task.cpp. 

LoRaE5.cpp has a function to get devEUI identifier from the gateway to be able to join. 

Ultra-Low power mode is entered in the beginning and all messages are sent with Low-Power wake bytes in the beginning. 