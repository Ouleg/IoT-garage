#pragma once

// Tipovi aktuatora koje podržavamo
typedef enum {
    ACT_FAN = 0,
    ACT_ALARM,
    ACT_FIRE_EXT,
    ACT_BARRIER
} actuator_type_t;

// Vrati ID aktuatora za zadatu zonu
const char* reg_actuator_id(const char* zone, actuator_type_t t);
