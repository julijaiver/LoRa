#ifndef STRUCTS_H
#define STRUCTS_H

enum setup_states {
    SET_AUTOON,
    JOIN_NETWORK,
    SETUP_DONE
};

enum data_type {
    PARTICULATE = 1,
    BME690 = 2,
    TEMPERATURE = 3
};

struct particulate_data {
    float pm25;
    float pm10;
};

struct bme690_data {
    float voc;
    float pressure;
    float humidity;
};

struct temperature_data {
    float temperature;
};

struct sensor_data {
    data_type type;
    union {
        struct particulate_data p_data;
        struct bme690_data b_data;
        struct temperature_data t_data;
    } data;
};

#endif //STRUCTS_H