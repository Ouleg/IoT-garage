#pragma once

typedef enum {
    SENSOR_CO = 0,
    SENSOR_TEMP,
    SENSOR_VIB
} sensor_type_t;

int sensors_run(sensor_type_t type, const char* zone, const char* dev_id, int period_sec);
