#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <mosquitto.h>
#include "cJSON.h"

#include "../include/config.h"
#include "../include/protocol.h"
#include "../include/registry.h"
#include "../actuators/actuators.h"
#include "ssdp_dev.h"


typedef struct {
    double temp_c, co_ppm, vib_g, tilt_deg;
    int have_temp, have_co, have_vib, have_tilt;
    int fire_active, struct_active;
    int fan_on, alarm_on, fireext_on, barrier_down;
} zone_state_t;

typedef struct zone_entry {
    char zone[32];
    zone_state_t st;
    struct zone_entry* next;
} zone_entry_t;

static zone_entry_t* gZones = 0;

static zone_entry_t* get_or_make_zone(const char* zone){
    zone_entry_t* p;
    for(p = gZones; p; p = p->next){
        if(strcmp(p->zone, zone) == 0) return p;
    }
    p = (zone_entry_t*)calloc(1, sizeof(*p));
    strncpy(p->zone, zone, sizeof(p->zone)-1);
    p->next = gZones; gZones = p;
    return p;
}

static void apply_rules(const char* zone, zone_state_t* S){
    int bad_fire = (S->have_temp && S->temp_c >= TEMP_BAD_C);
    int bad_co   = (S->have_co   && S->co_ppm >= CO_BAD_PPM);
    int bad_str  = ((S->have_vib && S->vib_g   >= VIB_BAD_G) ||
                    (S->have_tilt&& S->tilt_deg>= TILT_BAD_DEG));

    const char* fan_id     = reg_actuator_id(zone, ACT_FAN);
    const char* alarm_id   = reg_actuator_id(zone, ACT_ALARM);
    const char* fireext_id = reg_actuator_id(zone, ACT_FIRE_EXT);
    const char* barrier_id = reg_actuator_id(zone, ACT_BARRIER);

    /* FIRE */
    if(bad_fire){
        if(!S->alarm_on){ act_set_alarm(zone, alarm_id, 1); S->alarm_on = 1; }
        if(!S->fireext_on){ act_set_fire_ext(zone, fireext_id, 1); S->fireext_on = 1; }
        if(!S->barrier_down){ act_set_barrier_down(barrier_id, 1); S->barrier_down = 1; }
        S->fire_active = 1;
    } else {
        if(S->alarm_on){ act_set_alarm(zone, alarm_id, 0); S->alarm_on = 0; }
        S->fire_active = 0;
    }

    /* STRUCTURE */
    if(bad_str){
        if(!S->alarm_on){ act_set_alarm(zone, alarm_id, 1); S->alarm_on = 1; }
        if(!S->barrier_down){ act_set_barrier_down(barrier_id, 1); S->barrier_down = 1; }
        S->struct_active = 1;
    } else {
        if(!S->fire_active && S->alarm_on){ act_set_alarm(zone, alarm_id, 0); S->alarm_on = 0; }
        S->struct_active = 0;
    }

    /* AIR (CO) */
    if(bad_co){
        if(!S->fan_on){ act_set_fan(zone, fan_id, 1); S->fan_on = 1; }
    } else {
        if(S->fan_on){ act_set_fan(zone, fan_id, 0); S->fan_on = 0; }
    }
}

static void on_message(struct mosquitto* m, void* ud, const struct mosquitto_message* msg){
    (void)m; (void)ud;
    if(!msg->payload || msg->payloadlen <= 0) return;

    char zone[32] = {0}, type[32] = {0};
    if(sscanf(msg->topic, "garage/%31[^/]/sensors/%31s", zone, type) != 2) return;

    zone_entry_t* Z = get_or_make_zone(zone);
    zone_state_t* S = &Z->st;

    cJSON* root = cJSON_Parse((const char*)msg->payload);
    if(!root) return;

    if(strcmp(type,"temp")==0){
        cJSON* t = cJSON_GetObjectItemCaseSensitive(root, "temperature");
        if(!t) t = cJSON_GetObjectItemCaseSensitive(root, "value");
        if(cJSON_IsNumber(t)){ S->temp_c = t->valuedouble; S->have_temp = 1; }
    } else if(strcmp(type,"co")==0){
        cJSON* c = cJSON_GetObjectItemCaseSensitive(root, "COppm");
        if(!c) c = cJSON_GetObjectItemCaseSensitive(root, "value");
        if(cJSON_IsNumber(c)){ S->co_ppm = c->valuedouble; S->have_co = 1; }
    } else if(strcmp(type,"vibration")==0){
        cJSON* vg = cJSON_GetObjectItemCaseSensitive(root, "vib_g");
        cJSON* td = cJSON_GetObjectItemCaseSensitive(root, "tilt_deg");
        if(cJSON_IsNumber(vg)){ S->vib_g = vg->valuedouble; S->have_vib = 1; }
        if(cJSON_IsNumber(td)){ S->tilt_deg = td->valuedouble; S->have_tilt = 1; }
        /* fallback: {"value": 0.7} */
        if(!S->have_vib){
            cJSON* v = cJSON_GetObjectItemCaseSensitive(root, "value");
            if(cJSON_IsNumber(v)){ S->vib_g = v->valuedouble; S->have_vib = 1; }
        }
    }
    cJSON_Delete(root);

    apply_rules(zone, S);
}

static void on_connect(struct mosquitto* m, void* ud, int rc){
    (void)ud;
    printf("MQTT connected rc=%d\n", rc);
    mosquitto_subscribe(m, NULL, SUB_PATTERN_TEMP, 1);
    mosquitto_subscribe(m, NULL, SUB_PATTERN_CO,   1);
    mosquitto_subscribe(m, NULL, SUB_PATTERN_VIB,  1);
}

int controller_init(struct mosquitto* m){
    act_init(m);
    mosquitto_connect_callback_set(m, on_connect);
    mosquitto_message_callback_set(m, on_message);
    return 1;
}

void controller_start_loop(struct mosquitto* m){
    mosquitto_loop_forever(m, -1, 1);
}
