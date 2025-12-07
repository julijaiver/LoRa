#ifndef STRUCTS_H
#define STRUCTS_H

enum setup_states {
    SET_AUTOON,
    JOIN_NETWORK,
    SETUP_DONE
};

struct dummy_data {
    float temp;
    float humidity;
    float pressure;
    float gas;
};

#endif //STRUCTS_H