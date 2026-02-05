#ifndef STRUCTS_H
#define STRUCTS_H

enum data_type {
    SPS30 = 1,
    BMV080 = 2,
    BME690 = 3,
    TC74A2 = 4,
};

struct sps30_data {
    float pm25_numerical;
    float pm25_mass; 
    float pm10_numerical;   
    float pm10_mass;
};

struct bmv080_data {
    float pm25_numerical;
    float pm25_mass;    
    float pm10_numerical;
    float pm10_mass;
};

struct bme690_data {
    float voc;
    float pressure;
    float humidity;
};

struct tc74a2_data {
    float temperature;
};

struct sensor_data {
    data_type type;
    union {
        struct sps30_data sps_data;
        struct bmv080_data bmv_data;
        struct bme690_data bme_data;
        struct tc74a2_data t_data;
    } data;
};

#endif //STRUCTS_H