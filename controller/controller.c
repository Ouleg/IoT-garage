// controller/controller.c
// Robustna varijanta kontrolera (MQTT + cJSON) sa urednim vremenom i parsiranjem.

#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include <mosquitto.h>
#include "cJSON.h"

#include "../include/config.h"
#include "../include/protocol.h"
#include "../actuators/actuators.h"

// -------------------- util: vreme u ms (radi i bez -lrt) --------------------
static unsigned long long now_ms(void){
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    // C11: nema potrebe za -lrt
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (unsigned long long)ts.tv_sec * 1000ULL
         + (unsigned long long)ts.tv_nsec / 1000000ULL;
#else
    // Fallback: clock_gettime (traži -lrt na starijim glibc) ili gettimeofday
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        return (unsigned long long)ts.tv_sec * 1000ULL
             + (unsigned long long)ts.tv_nsec / 1000000ULL;
    } else {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (unsigned long long)tv.tv_sec * 1000ULL
             + (unsigned long long)tv.tv_usec / 1000ULL;
    }
#endif
}

// -------------------- globalno stanje --------------------
static volatile int g_run = 1;
static struct mosquitto* gM = NULL;
static const char* g_zone = DEFAULT_ZONE;

// poslednje viđeni podaci + heartbeat
typedef struct {
    uint64_t last_seen_ms;
    int      co_ppm;
    double   temp_c;
    double   tilt_deg;
    
    //za pracenje zadnjeg stanja
    uint64_t co_seen_ms;
    uint64_t temp_seen_ms;
    uint64_t tilt_seen_ms;

} sense_t;

static sense_t S = {0};

static int last_fan   = -1;
static int last_alarm = -1;
static int last_barrier = -1;


static void on_sigint(int s){ (void)s; g_run = 0; }

// -------------------- mali helperi --------------------
static int topic_ends_with(const char* topic, const char* suffix){
    if(!topic || !suffix) return 0;
    size_t lt = strlen(topic), ls = strlen(suffix);
    if (ls > lt) return 0;
    return strcmp(topic + (lt - ls), suffix) == 0;
}

static void log_kv(const char* k, const char* v){
    fprintf(stdout, "[%s] %s\n", k, v);
}

// -------------------- actuators helpers --------------------
static void set_fan_if_changed(int on){
    if(on != last_fan){
        act_set_fan(g_zone, "fan_1", on);
        last_fan = on;
        fprintf(stdout, "FAN -> %s\n", on ? "ON" : "OFF");
    }
}

static void set_alarm_if_changed(int on){
    if(on != last_alarm){
        act_set_alarm(g_zone, "alarm_1", on);
        last_alarm = on;
        fprintf(stdout, "ALARM -> %s\n", on ? "ON" : "OFF");
    }
}

static void set_barrier_if_changed(int down){
    if(down != last_barrier){
        act_set_barrier_down("barrier_1", down);
        last_barrier = down;
        fprintf(stdout, "BARRIER -> %s\n", down ? "DOWN" : "UP");
    }
}

static inline int is_fresh(uint64_t seen_ms, uint64_t now, uint64_t stale_sec){
    return seen_ms && (now - seen_ms) <= stale_sec*1000ULL;
}


// -------------------- subscribe --------------------
static void subscribe_topics(struct mosquitto* m, const char* zone){
    char t_co[128], t_temp[128], t_vib[128];
    snprintf(t_co,   sizeof t_co,   TOPIC_SENSOR_FMT, zone, "co");
    snprintf(t_temp, sizeof t_temp, TOPIC_SENSOR_FMT, zone, "temp");
    snprintf(t_vib,  sizeof t_vib,  TOPIC_SENSOR_FMT, zone, "vibration");

    // QoS 1 da poruke ne promaknu
    mosquitto_subscribe(m, NULL, t_co,   1);
    mosquitto_subscribe(m, NULL, t_temp, 1);
    mosquitto_subscribe(m, NULL, t_vib,  1);

    fprintf(stdout, "Subscribing: %s | %s | %s\n", t_co, t_temp, t_vib);
}

// -------------------- parsers (sigurno, tolerantno) --------------------
// CO:   {"id":"co_1","group":"sensors","COService":{"COppm":165}}
static void parse_co(const char* payload){
    if(!payload) return;
    cJSON* root = cJSON_Parse(payload);
    if(!root) return;

    // (opciono) potvrdi group=="sensors"
    const cJSON* group = cJSON_GetObjectItemCaseSensitive(root, "group");
    if (!cJSON_IsString(group) || strcasecmp(group->valuestring, "sensors") != 0) {
        cJSON_Delete(root); return;
    }

    cJSON* svc = cJSON_GetObjectItem(root, JSON_SERVICE_CO);
    if(cJSON_IsObject(svc)){
        cJSON* v = cJSON_GetObjectItem(svc, JSON_PARAM_CO);
        if(cJSON_IsNumber(v)) {
            S.co_ppm = v->valueint;
            S.last_seen_ms = now_ms();
            S.co_seen_ms   = S.last_seen_ms;
            fprintf(stdout, "COppm=%d\n", S.co_ppm);
        }
    }
    cJSON_Delete(root);
}

// TEMP: {"id":"t_1","group":"sensors","TempService":{"temperature":72.5}}
static void parse_temp(const char* payload){
    if(!payload) return;
    cJSON* root = cJSON_Parse(payload);
    if(!root) return;

    const cJSON* group = cJSON_GetObjectItemCaseSensitive(root, "group");
    if (!cJSON_IsString(group) || strcasecmp(group->valuestring, "sensors") != 0) {
        cJSON_Delete(root); return;
    }

    cJSON* svc = cJSON_GetObjectItem(root, JSON_SERVICE_TEMP);
    if(cJSON_IsObject(svc)){
        cJSON* v = cJSON_GetObjectItem(svc, JSON_PARAM_TEMP);
        if(cJSON_IsNumber(v)) {
            S.temp_c = v->valuedouble;
            S.last_seen_ms = now_ms();
            S.temp_seen_ms = S.last_seen_ms;
            fprintf(stdout, "TEMP=%.2f C\n", S.temp_c);
        }
    }
    cJSON_Delete(root);
}

// VIB:  {"id":"v_1","group":"sensors","VibService":{"vib_g":0.7,"tilt_deg":12.3}}
static void parse_vib(const char* payload){
    if(!payload) return;
    cJSON* root = cJSON_Parse(payload);
    if(!root) return;

    const cJSON* group = cJSON_GetObjectItemCaseSensitive(root, "group");
    if (!cJSON_IsString(group) || strcasecmp(group->valuestring, "sensors") != 0) {
        cJSON_Delete(root); return;
    }

    cJSON* svc = cJSON_GetObjectItem(root, JSON_SERVICE_VIB);
    if(cJSON_IsObject(svc)){
        // vib_g je opcioni, za odluku koristiš tilt i/ili temp
        const cJSON* tilt = cJSON_GetObjectItem(svc, "tilt_deg");
        if(cJSON_IsNumber(tilt)) {
            S.tilt_deg = tilt->valuedouble;
            S.last_seen_ms = now_ms();
            S.tilt_seen_ms = S.last_seen_ms;
            fprintf(stdout, "TILT=%.2f deg\n", S.tilt_deg);
        }
    }
    cJSON_Delete(root);
}

// -------------------- decision logic --------------------
static void apply_logic(void){
    const uint64_t now = now_ms();

    // FAN: visok co ili visoka temperatura
    int want_fan = 0;
    if (is_fresh(S.co_seen_ms,   now, SENSOR_STALE_SEC) && S.co_ppm > CO_THRESHOLD_PPM)          want_fan = 1;
    if (is_fresh(S.temp_seen_ms, now, SENSOR_STALE_SEC) && S.temp_c  > TEMP_THRESHOLD_C)     want_fan = 1;
    set_fan_if_changed(want_fan);

    // ALARM: visoka temperatura ili veliki tilt
    int want_alarm = 0;
    if (is_fresh(S.temp_seen_ms, now, SENSOR_STALE_SEC) && S.temp_c  > TEMP_THRESHOLD_C)         want_alarm = 1;
    if (is_fresh(S.tilt_seen_ms, now, SENSOR_STALE_SEC) && S.tilt_deg > TILT_THRESHOLD_DEG)      want_alarm = 1;
    set_alarm_if_changed(want_alarm);

    // BARRIER: veliki tilt
    int want_barrier_down = 1;
    if (is_fresh(S.tilt_seen_ms, now, SENSOR_STALE_SEC) && S.tilt_deg > TILT_THRESHOLD_DEG)      want_barrier_down = 0;
    set_barrier_if_changed(want_barrier_down);


}

// -------------------- MQTT callbacks --------------------
static void on_connect(struct mosquitto *m, void *obj, int rc){
    (void)obj;
    if(rc == 0){
        log_kv("MQTT", "connected");
        subscribe_topics(m, g_zone);
    } else {
        fprintf(stderr,"MQTT connect failed rc=%d\n", rc);
    }
}

static void on_message(struct mosquitto *m, void *obj, const struct mosquitto_message *msg){
    (void)m; (void)obj;
    if(!msg || !msg->topic || !msg->payload) return;

    // Matč po tačnom sufiksu (stabilnije od golih strstr)
    if      (topic_ends_with(msg->topic, "/sensors/co"))         parse_co((const char*)msg->payload);
    else if (topic_ends_with(msg->topic, "/sensors/temp"))       parse_temp((const char*)msg->payload);
    else if (topic_ends_with(msg->topic, "/sensors/vibration"))  parse_vib((const char*)msg->payload);

    apply_logic();
}

// -------------------- kontroler main --------------------
int controller_main(const char* host, int port, const char* zone){
    g_zone = zone;
    mosquitto_lib_init();
    gM = mosquitto_new("garage_ctrl", true, NULL);
    if(!gM){ fprintf(stderr,"mosquitto_new failed\n"); return 2; }

    mosquitto_connect_callback_set(gM, on_connect);
    mosquitto_message_callback_set(gM, on_message);

    if(mosquitto_connect(gM, host, port, MQTT_KEEPALIVE) != MOSQ_ERR_SUCCESS){
        fprintf(stderr,"MQTT connect error to %s:%d\n", host, port);
        mosquitto_destroy(gM); mosquitto_lib_cleanup();
        return 3;
    }

    // init actuators API sa istim mosq handlerom
    act_init(gM);

    // I/O petlja (sa blagim sleep-om)
    while(g_run){
        int rc = mosquitto_loop(gM, 100, 1);
        if (rc != MOSQ_ERR_SUCCESS) {
            // pokušaj reconnect (ne paniči odmah)
            mosquitto_reconnect_delay_set(gM, 1, 5, false);
            mosquitto_reconnect(gM);
        }
        // periodični safety check
        static uint64_t last = 0;
        uint64_t t = now_ms();
        if(t - last > 2000){ apply_logic(); last = t; }

        struct timespec sl = { .tv_sec = 0, .tv_nsec = 100 * 1000000 };
        nanosleep(&sl, NULL);
    }

    mosquitto_disconnect(gM);
    mosquitto_destroy(gM);
    mosquitto_lib_cleanup();
    return 0;
}

// -------------------- ugrađeni CLI (ako ne koristiš apps/cli.c) --------------------
#ifndef BUILD_AS_LIB
static void usage(const char* argv0){
    fprintf(stderr, "Usage: %s [host] [port] [zone]\n", argv0);
    fprintf(stderr, "Defaults: host=%s port=%d zone=%s\n", MQTT_HOST, MQTT_PORT, DEFAULT_ZONE);
}
int main(int argc, char** argv){
    signal(SIGINT, on_sigint);

    const char* host = MQTT_HOST;
    int         port = MQTT_PORT;
    const char* zone = DEFAULT_ZONE;

    if(argc >= 2 && strcmp(argv[1], "-h") == 0){ usage(argv[0]); return 0; }
    if(argc >= 2) host = argv[1];
    if(argc >= 3) port = atoi(argv[2]);
    if(argc >= 4) zone = argv[3];

    g_zone = zone;

    printf("Controller → %s:%d, zone=%s\n", host, port, zone);
    return controller_main(host, port, zone);
}
#endif
