#pragma once
#include <stdbool.h>

typedef struct {
    char id[64];
    char group[32];
    char zone[16];
    char type[32];   // "sensor-co", "actuator-fan", ...
    // poslednji opis/stanje kao JSON string (keš)
    char info_json[512];
    char state_json[512];
    long last_seen_ssdp; // epoch sec
} device_t;

bool registry_init(void);
bool registry_upsert(const device_t* d);
bool registry_set_info(const char* id, const char* json);
bool registry_set_state(const char* id, const char* json);
bool registry_get_info(const char* id, char* out, int outsz);
bool registry_get_state(const char* id, char* out, int outsz);
int  registry_list(char* out, int outsz, bool list_state); // * ili listing
bool registry_mark_seen(const char* id, long now);
int  registry_prune_expired(long now, long max_age);
