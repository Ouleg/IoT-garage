// sim/garage_sim.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include <mosquitto.h>
#include "cJSON.h"

#include "../include/config.h"
#include "../include/protocol.h"
#include "../include/garage.h"

static volatile int g_run = 1;
static struct mosquitto* gM = NULL;

static const char* ZONE   = DEFAULT_ZONE;
static const char* FAN_ID = SIM_FAN_ID;
static const char* ALM_ID = SIM_ALARM_ID;

static void on_sigint(int s){ (void)s; g_run = 0; }

/* ========== JSON publish helper ========== */
static void pub_json(const char* topic, cJSON* root, int retain){
    char* s = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if(!s) return;
    mosquitto_publish(gM, NULL, topic, (int)strlen(s), s, 1, retain?1:0);
    free(s);
}

/* ========== Actuator echo (cmd -> state) ========== */
static void on_message(struct mosquitto* m, void* obj, const struct mosquitto_message* msg){
    (void)m; (void)obj;
    if(!msg || !msg->topic || !msg->payload) return;

    // Simuliramo da aktuator odmah potvrdi state
    const char* topic = msg->topic;
    const char* payload = (const char*)msg->payload;

    // fan cmd: garage/<zone>/cmd/fan
    if (strstr(topic, "/cmd/fan")) {
        char t_state[128];
        // state: garage/<zone>/state/fan/<id>
        snprintf(t_state, sizeof t_state, "garage/%s/state/fan/%s", ZONE, FAN_ID);
        // payload već sadrži {"id":"fan_1","state":"ON" ...}
        mosquitto_publish(gM, NULL, t_state, (int)strlen(payload), payload, 1, 0);
        fprintf(stdout, "[sim-actuator] FAN state echoed: %s\n", payload);
    }
    // alarm cmd: garage/<zone>/cmd/alarm
    else if (strstr(topic, "/cmd/alarm")) {
        char t_state[128];
        snprintf(t_state, sizeof t_state, "garage/%s/state/alarm/%s", ZONE, ALM_ID);
        mosquitto_publish(gM, NULL, t_state, (int)strlen(payload), payload, 1, 0);
        fprintf(stdout, "[sim-actuator] ALARM state echoed: %s\n", payload);
    }
}

/* ========== Sensor publishers (identičan format tvom sensors.c) ========== */
static void publish_co(const char* zone, const char* id, int co_ppm){
    char topic[128];
    snprintf(topic, sizeof topic, TOPIC_SENSOR_FMT, zone, "co");
    cJSON* root = cJSON_CreateObject();
    cJSON* svc  = cJSON_CreateObject();
    cJSON_AddStringToObject(root, JSON_ID, id);
    cJSON_AddStringToObject(root, JSON_GROUP, "sensors");
    cJSON_AddNumberToObject(svc, JSON_PARAM_CO, co_ppm);
    cJSON_AddItemToObject(root, JSON_SERVICE_CO, svc);
    pub_json(topic, root, 0);
}
static void publish_temp(const char* zone, const char* id, double t_c){
    char topic[128];
    snprintf(topic, sizeof topic, TOPIC_SENSOR_FMT, zone, "temp");
    cJSON* root = cJSON_CreateObject();
    cJSON* svc  = cJSON_CreateObject();
    cJSON_AddStringToObject(root, JSON_ID, id);
    cJSON_AddStringToObject(root, JSON_GROUP, "sensors");
    cJSON_AddNumberToObject(svc, JSON_PARAM_TEMP, t_c);
    cJSON_AddItemToObject(root, JSON_SERVICE_TEMP, svc);
    pub_json(topic, root, 0);
}
static void publish_vib(const char* zone, const char* id, double vib_g, double tilt_deg){
    char topic[128];
    snprintf(topic, sizeof topic, TOPIC_SENSOR_FMT, zone, "vibration");
    cJSON* root = cJSON_CreateObject();
    cJSON* svc  = cJSON_CreateObject();
    cJSON_AddStringToObject(root, JSON_ID, id);
    cJSON_AddStringToObject(root, JSON_GROUP, "sensors");
    cJSON_AddNumberToObject(svc, "vib_g",    vib_g);
    cJSON_AddNumberToObject(svc, "tilt_deg", tilt_deg);
    cJSON_AddItemToObject(root, JSON_SERVICE_VIB, svc);
    pub_json(topic, root, 0);
}

/* ========== Scenario funkcije ========== */
// t je “globalno trajanje” (sekunde od starta simulacije)
static int   co_profile(int t){
    int p1 = PHASE1_NORMAL_SEC;
    int p2 = p1 + PHASE2_CO_RISE_SEC;
    int p3 = p2 + PHASE3_TEMP_SPIKE;
    int p4 = p3 + PHASE4_VIB_EVENT;
    // p5 = recovery do kraja

    if (t <= p1) {                   // normalno: 40..80
        return 40 + (t % 40);
    } else if (t <= p2) {            // raste do 180
        int dt = t - p1;
        return CLAMP(80 + dt*4, 80, 180);
    } else if (t <= p3) {            // ostane visok
        return 170 + (t%2?5:0);
    } else if (t <= p4) {            // polako opada
        int dt = t - p3;
        return CLAMP(160 - dt*4, 80, 160);
    } else {                         // recovery 70..50
        int dt = t - p4;
        return CLAMP(70 - dt*2, 40, 70);
    }
}
static double temp_profile(int t){
    int p1 = PHASE1_NORMAL_SEC;
    int p2 = p1 + PHASE2_CO_RISE_SEC;
    int p3 = p2 + PHASE3_TEMP_SPIKE;
    int p4 = p3 + PHASE4_VIB_EVENT;

    if (t <= p2) {                   // normalno ~ 35..50
        return 35.0 + (t % 15);
    } else if (t <= p3) {            // spike do 72
        int dt = t - p2;
        return CLAMP(50.0 + dt*1.5, 50.0, 72.0);
    } else if (t <= p4) {            // drži se visoko
        return 68.0 + (t%2?2.0:0.0);
    } else {                         // recovery ka 40
        int dt = t - p4;
        double v = 65.0 - dt*1.0;
        if (v < 40.0) v = 40.0;
        return v;
    }
}
static double tilt_profile(int t){
    int p1 = PHASE1_NORMAL_SEC;
    int p2 = p1 + PHASE2_CO_RISE_SEC;
    int p3 = p2 + PHASE3_TEMP_SPIKE;
    int p4 = p3 + PHASE4_VIB_EVENT;

    if (t <= p3) {                   // mirno 0.5..3
        return 0.5 + (t%5)*0.5;
    } else if (t <= p4) {            // event: 12..15 deg (preko praga 10)
        return 12.0 + (t%4);
    } else {                         // pad nazad 5..2
        return 5.0 - (t%4)*0.7;
    }
}
static double vib_profile(int t){
    // čisto kozmetika
    return (t%10==5)?0.8:0.15+(t%5)*0.05;
}

/* ========== MAIN ========== */
int main(int argc, char** argv){
    const char* host = MQTT_HOST; // sa laptopa stavi u config.h IP RPi-ja
    int port = MQTT_PORT;
    const char* zone = DEFAULT_ZONE;

    // Flags: --host --port --zone --fan --siren
    for(int i=1;i<argc;i++){
        if(strcmp(argv[i],"--host")==0 && i+1<argc) host = argv[++i];
        else if(strcmp(argv[i],"--port")==0 && i+1<argc) port = atoi(argv[++i]);
        else if(strcmp(argv[i],"--zone")==0 && i+1<argc) zone = argv[++i];
        else if(strcmp(argv[i],"--fan")==0 && i+1<argc)  FAN_ID = argv[++i];
        else if(strcmp(argv[i],"--siren")==0 && i+1<argc) ALM_ID = argv[++i];
    }
    ZONE = zone;

    signal(SIGINT, on_sigint);
    mosquitto_lib_init();
    gM = mosquitto_new("garage_sim", true, NULL);
    if(!gM){ fprintf(stderr,"mosquitto_new fail\n"); return 2; }

    // subscribe na komande pa echo state
    char t_cmd_fan[128], t_cmd_alm[128];
    snprintf(t_cmd_fan, sizeof t_cmd_fan, "garage/%s/cmd/fan",   ZONE);
    snprintf(t_cmd_alm, sizeof t_cmd_alm, "garage/%s/cmd/alarm", ZONE);

    mosquitto_message_callback_set(gM, on_message);
    if(mosquitto_connect(gM, host, port, 60) != MOSQ_ERR_SUCCESS){
        fprintf(stderr,"connect fail\n"); return 3;
    }
    mosquitto_subscribe(gM, NULL, t_cmd_fan, 1);
    mosquitto_subscribe(gM, NULL, t_cmd_alm, 1);

    fprintf(stdout,"[sim] connected to %s:%d zone=%s\n", host, port, ZONE);
    fprintf(stdout,"[sim] scenario: normal -> CO rise -> TEMP spike -> VIB event -> recovery\n");

    int t=0, co_tick=0, t_tick=0, v_tick=0;
    while(g_run){
        // publish po periodima
        if (co_tick % PERIOD_CO_SEC == 0)
            publish_co(ZONE, "co_1", co_profile(t));
        if (t_tick % PERIOD_TEMP_SEC == 0)
            publish_temp(ZONE, "t_1", temp_profile(t));
        if (v_tick % PERIOD_VIB_SEC == 0)
            publish_vib(ZONE, "v_1", vib_profile(t), tilt_profile(t));

        mosquitto_loop(gM, 50, 1);
        sleep(1);
        ++t; ++co_tick; ++t_tick; ++v_tick;

        // završi posle zbir trajanja (opcionalno)
        int total = PHASE1_NORMAL_SEC + PHASE2_CO_RISE_SEC + PHASE3_TEMP_SPIKE + PHASE4_VIB_EVENT + PHASE5_RECOVERY;
        if (t > total) break;
    }

    mosquitto_disconnect(gM);
    mosquitto_destroy(gM);
    mosquitto_lib_cleanup();
    fprintf(stdout,"[sim] done.\n");
    return 0;
}
