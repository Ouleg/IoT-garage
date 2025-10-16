// controller/controller.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
static unsigned long long now_ms(void){
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (unsigned long long)ts.tv_sec * 1000ULL + (unsigned long long)ts.tv_nsec / 1000000ULL;
}

#include <mosquitto.h>
#include "cJSON.h"

#include "../include/config.h"
#include "../include/protocol.h"
#include "../actuators/actuators.h"

static volatile int g_run = 1;
static struct mosquitto* gM = NULL;
static const char* g_zone = DEFAULT_ZONE;

// poslednje viđeni podaci + heartbeat
typedef struct {
    uint64_t last_seen_ms;
    int      co_ppm;
    double   temp_c;
    double   tilt_deg;
} sense_t;

static sense_t S = {0};

static void on_sigint(int s){ (void)s; g_run = 0; }

static int last_fan   = -1;
static int last_alarm = -1;

static void set_fan_if_changed(int on){
    if(on != last_fan){
        act_set_fan(g_zone, "fan_1", on);
        last_fan = on;
    }
}

static void set_alarm_if_changed(int on){
    if(on != last_alarm){
        act_set_alarm(g_zone, "alarm_1", on);
        last_alarm = on;
    }
}


static void subscribe_topics(struct mosquitto* m, const char* zone){
    char t_co[128], t_temp[128], t_vib[128];
    snprintf(t_co,   sizeof t_co,   TOPIC_SENSOR_FMT, zone, "co");
    snprintf(t_temp, sizeof t_temp, TOPIC_SENSOR_FMT, zone, "temp");
    snprintf(t_vib,  sizeof t_vib,  TOPIC_SENSOR_FMT, zone, "vibration");

    mosquitto_subscribe(m, NULL, t_co,   1);
    mosquitto_subscribe(m, NULL, t_temp, 1);
    mosquitto_subscribe(m, NULL, t_vib,  1);

    fprintf(stdout, "Subscribing: %s, %s, %s\n", t_co, t_temp, t_vib);
}

// ---- Parsers (tvoj JSON format) ----
// CO:  {"id":"co_1","group":"sensors","COService":{"COppm":165}}
static void parse_co(const char* payload){
    cJSON* root = cJSON_Parse(payload);
    if(!root) return;
    cJSON* svc = cJSON_GetObjectItem(root, JSON_SERVICE_CO);
    if(cJSON_IsObject(svc)){
        cJSON* v = cJSON_GetObjectItem(svc, JSON_PARAM_CO);
        if(cJSON_IsNumber(v)) S.co_ppm = v->valueint;
        S.last_seen_ms = now_ms();
    }
    cJSON_Delete(root);
}

// TEMP: {"id":"t_1","group":"sensors","TempService":{"temperature":72.5}}
static void parse_temp(const char* payload){
    cJSON* root = cJSON_Parse(payload);
    if(!root) return;
    cJSON* svc = cJSON_GetObjectItem(root, JSON_SERVICE_TEMP);
    if(cJSON_IsObject(svc)){
        cJSON* v = cJSON_GetObjectItem(svc, JSON_PARAM_TEMP);
        if(cJSON_IsNumber(v)) S.temp_c = v->valuedouble;
        S.last_seen_ms = now_ms();
    }
    cJSON_Delete(root);
}

// VIB:  {"id":"v_1","group":"sensors","VibService":{"vib_g":0.7,"tilt_deg":12.3}}
static void parse_vib(const char* payload){
    cJSON* root = cJSON_Parse(payload);
    if(!root) return;
    cJSON* svc = cJSON_GetObjectItem(root, JSON_SERVICE_VIB);
    if(cJSON_IsObject(svc)){
        cJSON* tilt = cJSON_GetObjectItem(svc, "tilt_deg");
        if(cJSON_IsNumber(tilt)) S.tilt_deg = tilt->valuedouble;
        S.last_seen_ms = now_ms();
    }
    cJSON_Delete(root);
}

// ---- Decision logic ----
static void apply_logic(void){
    uint64_t now = now_ms();

    // fail-safe: ako nema svežih merenja -> ugasi sve (jednom)
    if (S.last_seen_ms && (now - S.last_seen_ms) > (uint64_t)SENSOR_STALE_SEC*1000ULL) {
        set_fan_if_changed(0);
        set_alarm_if_changed(0);
        return;
    }

    // CO -> FAN (samo kad pređe prag ili se vrati ispod)
    int want_fan = (S.co_ppm > CO_THRESHOLD_PPM) ? 1 : 0;
    set_fan_if_changed(want_fan);

    // TEMP/TILT -> ALARM (isti princip)
    int want_alarm = (S.temp_c > TEMP_THRESHOLD_C) ? 1 : 0;
    if (S.tilt_deg > TILT_THRESHOLD_DEG) want_alarm = 1; // opciono: tilt pojačava alarm
    set_alarm_if_changed(want_alarm);
}


// ---- MQTT callbacks ----
static void on_connect(struct mosquitto *m, void *obj, int rc){
    (void)obj;
    if(rc == 0){
        fprintf(stdout,"MQTT connected\n");
        subscribe_topics(m, g_zone);
    } else {
        fprintf(stderr,"MQTT connect failed rc=%d\n", rc);
    }
}

static void on_message(struct mosquitto *m, void *obj, const struct mosquitto_message *msg){
    (void)m; (void)obj;
    if(!msg || !msg->topic || !msg->payload) return;

    // Poklapaj po suffix-u topika (brže i jednostavno)
    if(strstr(msg->topic, "/sensors/co"))        parse_co((const char*)msg->payload);
    else if(strstr(msg->topic, "/sensors/temp")) parse_temp((const char*)msg->payload);
    else if(strstr(msg->topic, "/sensors/vibration")) parse_vib((const char*)msg->payload);

    apply_logic();
}

int controller_main(const char* host, int port, const char* zone){
    mosquitto_lib_init();
    gM = mosquitto_new("garage_ctrl", 1, NULL);
    if(!gM){ fprintf(stderr,"mosquitto_new failed\n"); return 2; }

    mosquitto_connect_callback_set(gM, on_connect);
    mosquitto_message_callback_set(gM, on_message);

    if(mosquitto_connect(gM, host, port, MQTT_KEEPALIVE) != MOSQ_ERR_SUCCESS){
        fprintf(stderr,"MQTT connect error\n");
        mosquitto_destroy(gM); mosquitto_lib_cleanup();
        return 3;
    }

    // init actuators API sa istim mosq handlerom
    act_init(gM);

    // I/O petlja
    while(g_run){
        mosquitto_loop(gM, 100, 1);
        // može i periodični safety check:
        static uint64_t last = 0;
        uint64_t now = now_ms();
        if(now - last > 2000){ apply_logic(); last = now; }
        struct timespec sl = {.tv_sec=0,.tv_nsec=100*1000000};
        nanosleep(&sl, NULL);
    }

    mosquitto_disconnect(gM);
    mosquitto_destroy(gM);
    mosquitto_lib_cleanup();
    return 0;
}

// Minimalni CLI (ako ne koristiš zasebni apps/cli.c)
#ifndef BUILD_AS_LIB
int main(int argc, char** argv){
    signal(SIGINT, on_sigint);
    const char* host = MQTT_HOST;
    int port = MQTT_PORT;
    const char* zone = DEFAULT_ZONE;

    if(argc>=2) host = argv[1];
    if(argc>=3) port = atoi(argv[2]);
    if(argc>=4) zone = argv[3];
    g_zone = zone;

    printf("Controller → %s:%d, zone=%s\n", host, port, zone);
    return controller_main(host, port, zone);
}
#endif
