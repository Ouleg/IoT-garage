#include <string.h>
#include "../include/registry.h"

// Tabela zona i pripadajućih aktuatora
typedef struct {
    const char* zone;       // npr. "zone1"
    const char* fan_id;     // npr. "fan_1"
    const char* alarm_id;   // npr. "alarm_1"
    const char* fireext_id; // npr. "fireext_1"
    const char* barrier_id; // npr. "barrier_1"
} zone_row_t;

static const zone_row_t TBL[] = {
    { "zone1", "fan_1", "alarm_1", "fireext_1", "barrier_1" },
    { "zone2", "fan_2", "alarm_2", "fireext_2", "barrier_1" },
    { 0, 0, 0, 0, 0 }
};

static const zone_row_t* find(const char* zone){
    int i;
    for(i = 0; TBL[i].zone; ++i){
        if(strcmp(TBL[i].zone, zone) == 0) return &TBL[i];
    }
    return &TBL[0]; // fallback
}

const char* reg_actuator_id(const char* zone, actuator_type_t t){
    const zone_row_t* e = find(zone);
    switch(t){
        case ACT_FAN:      return e->fan_id;
        case ACT_ALARM:    return e->alarm_id;
        case ACT_FIRE_EXT: return e->fireext_id;
        case ACT_BARRIER:  return e->barrier_id;
        default:           return "unknown";
    }
}
