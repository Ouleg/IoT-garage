// actuators.c
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <mosquitto.h>
#include "cJSON.h"
#include "../include/protocol.h"   // očekuje se TOPIC_CMD_FMT i eventualno JSON_ID="id"

static struct mosquitto* gM = NULL;

// Jedinstveni helper: objavi JSON i očisti resurse.
// retain: 0/1
static void publish_json(const char* topic, cJSON* obj, int retain){
    if(!topic || !obj) { if(obj) cJSON_Delete(obj); return; }
    char* s = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    if(!s) return;
    if(gM){
        // QoS=1, retain po potrebi
        mosquitto_publish(gM, NULL, topic, (int)strlen(s), s, 1, retain ? true : false);
    }
    free(s);
}

void act_init(struct mosquitto* m){ gM = m; }

// ============ FAN ============
void act_set_fan(const char* zone, const char* id, int on){
    char t_state[128], t_cmd[128];
    const char* st = on ? "ON" : "OFF";

    // state: garage/<zone>/state/fan/<id>
    snprintf(t_state, sizeof t_state, "garage/%s/state/fan/%s", zone, id);
    // cmd:   garage/<zone>/cmd/fan
    snprintf(t_cmd,   sizeof t_cmd,   TOPIC_CMD_FMT,            zone, "fan");

    cJSON *s = cJSON_CreateObject();
    cJSON_AddStringToObject(s, "id", id);
    cJSON_AddStringToObject(s, "state", st);
    cJSON_AddNumberToObject(s, "ts", (double)time(NULL));
    publish_json(t_state, s, 0);

    cJSON *c = cJSON_CreateObject();
    cJSON_AddStringToObject(c, "id", id);
    cJSON_AddStringToObject(c, "state", st);
    publish_json(t_cmd, c, 0);
}

// ============ ALARM ============
void act_set_alarm(const char* zone, const char* id, int on){
    char t_state[128], t_cmd[128];
    const char* st = on ? "ON" : "OFF";

    snprintf(t_state, sizeof t_state, "garage/%s/state/alarm/%s", zone, id);
    snprintf(t_cmd,   sizeof t_cmd,   TOPIC_CMD_FMT,            zone, "alarm");

    cJSON *s = cJSON_CreateObject();
    cJSON_AddStringToObject(s, "id", id);
    cJSON_AddStringToObject(s, "state", st);
    cJSON_AddNumberToObject(s, "ts", (double)time(NULL));
    publish_json(t_state, s, 0);

    cJSON *c = cJSON_CreateObject();
    cJSON_AddStringToObject(c, "id", id);
    cJSON_AddStringToObject(c, "state", st);
    publish_json(t_cmd, c, 0);
}

// ============ FIRE EXTINGUISHER ============
void act_set_fire_ext(const char* zone, const char* id, int on){
    char t_state[128], t_cmd[128];
    const char* st = on ? "ON" : "OFF";

    snprintf(t_state, sizeof t_state, "garage/%s/state/fire_ext/%s", zone, id);
    snprintf(t_cmd,   sizeof t_cmd,   TOPIC_CMD_FMT,                 zone, "fire_ext");

    cJSON *s = cJSON_CreateObject();
    cJSON_AddStringToObject(s, "id", id);
    cJSON_AddStringToObject(s, "state", st);
    cJSON_AddNumberToObject(s, "ts", (double)time(NULL));
    publish_json(t_state, s, 0);

    cJSON *c = cJSON_CreateObject();
    cJSON_AddStringToObject(c, "id", id);
    cJSON_AddStringToObject(c, "state", st);
    publish_json(t_cmd, c, 0);
}

// ============ BARRIER (global entry) ============
void act_set_barrier_down(const char* id, int down){
    char t_state[128], t_cmd[128];
    const char* st = down ? "DOWN" : "UP";

    // state: garage/entry/state/barrier/<id>
    snprintf(t_state, sizeof t_state, "garage/entry/state/barrier/%s", id);
    // cmd:   garage/entry/cmd/barrier
    snprintf(t_cmd,   sizeof t_cmd,   "garage/entry/cmd/%s", "barrier");

    cJSON *s = cJSON_CreateObject();
    cJSON_AddStringToObject(s, "id", id);
    cJSON_AddStringToObject(s, "state", st);
    cJSON_AddNumberToObject(s, "ts", (double)time(NULL));
    publish_json(t_state, s, 0);

    cJSON *c = cJSON_CreateObject();
    cJSON_AddStringToObject(c, "id", id);
    cJSON_AddStringToObject(c, "state", st);
    publish_json(t_cmd, c, 0);
}
