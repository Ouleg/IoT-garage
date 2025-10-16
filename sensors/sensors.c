#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mosquitto.h>
#include "cJSON.h"

#include "../include/config.h"
#include "../include/protocol.h"
#include "../include/ssdp_dev.h"
#include "sensors.h"

#ifndef MQTT_QOS
#define MQTT_QOS 1
#endif

static struct mosquitto* mq = 0;

/* util */
static void publish_json(const char* topic, cJSON* obj, int retain){
    char* s = cJSON_PrintUnformatted(obj);
    mosquitto_publish(mq, NULL, topic, (int)strlen(s), s, MQTT_QOS, retain);
    free(s);
    cJSON_Delete(obj);
}

static void mqtt_connect_with_lwt(const char* dev_id){
    char status_topic[128];

    mosquitto_lib_init();
    mq = mosquitto_new(dev_id, 1, NULL);
    if(!mq){ fprintf(stderr,"mosquitto_new failed\n"); exit(2); }

    snprintf(status_topic, sizeof status_topic, TOPIC_STATUS_FMT, dev_id);
    mosquitto_will_set(mq, status_topic, 7, "offline", MQTT_QOS, 1);

    if(mosquitto_connect(mq, MQTT_HOST, MQTT_PORT, MQTT_KEEPALIVE) != MOSQ_ERR_SUCCESS){
        fprintf(stderr,"MQTT connect failed\n"); exit(3);
    }
    mosquitto_publish(mq, NULL, status_topic, 6, "online", MQTT_QOS, 1);
}

static void publish_info(const char* zone, const char* dev_id, sensor_type_t type){
    char topic[128];
    cJSON* root = cJSON_CreateObject();

    snprintf(topic, sizeof topic, "garage/%s/info/%s", zone, dev_id);
    cJSON_AddStringToObject(root, JSON_ID, dev_id);
    cJSON_AddStringToObject(root, JSON_GROUP, "sensors");

    if(type == SENSOR_CO){
        cJSON* svc = cJSON_CreateObject();
        cJSON_AddStringToObject(svc, JSON_PARAM_CO, "0~1000|r");
        cJSON_AddItemToObject(root, JSON_SERVICE_CO, svc);
    } else if(type == SENSOR_TEMP){
        cJSON* svc = cJSON_CreateObject();
        cJSON_AddStringToObject(svc, JSON_PARAM_TEMP, "-20~120|r");
        cJSON_AddItemToObject(root, JSON_SERVICE_TEMP, svc);
    } else {
        cJSON* svc = cJSON_CreateObject();
        cJSON_AddStringToObject(svc, "vib_g", "0.0~2.0|r");
        cJSON_AddStringToObject(svc, "tilt_deg", "0~30|r");
        cJSON_AddItemToObject(root, "VibService", svc);
    }
    publish_json(topic, root, 1);
}

static void publish_state_co(const char* zone, const char* dev_id, int co_ppm){
    char topic[128];
    cJSON* root = cJSON_CreateObject();
    cJSON* svc  = cJSON_CreateObject();

    snprintf(topic, sizeof topic, TOPIC_SENSOR_FMT, zone, "co");
    cJSON_AddStringToObject(root, JSON_ID, dev_id);
    cJSON_AddStringToObject(root, JSON_GROUP, "sensors");
    cJSON_AddNumberToObject(svc, JSON_PARAM_CO, co_ppm);
    cJSON_AddItemToObject(root, JSON_SERVICE_CO, svc);

    publish_json(topic, root, 0);
}

static void publish_state_temp(const char* zone, const char* dev_id, double t_c){
    char topic[128];
    cJSON* root = cJSON_CreateObject();
    cJSON* svc  = cJSON_CreateObject();

    snprintf(topic, sizeof topic, TOPIC_SENSOR_FMT, zone, "temp");
    cJSON_AddStringToObject(root, JSON_ID, dev_id);
    cJSON_AddStringToObject(root, JSON_GROUP, "sensors");
    cJSON_AddNumberToObject(svc, JSON_PARAM_TEMP, t_c);
    cJSON_AddItemToObject(root, JSON_SERVICE_TEMP, svc);

    publish_json(topic, root, 0);
}

static void publish_state_vib(const char* zone, const char* dev_id, double vib_g, double tilt_deg){
    char topic[128];
    cJSON* root = cJSON_CreateObject();
    cJSON* svc  = cJSON_CreateObject();

    snprintf(topic, sizeof topic, TOPIC_SENSOR_FMT, zone, "vibration");
    cJSON_AddStringToObject(root, JSON_ID, dev_id);
    cJSON_AddStringToObject(root, JSON_GROUP, "sensors");
    cJSON_AddNumberToObject(svc, "vib_g", vib_g);
    cJSON_AddNumberToObject(svc, "tilt_deg", tilt_deg);
    cJSON_AddItemToObject(root, "VibService", svc);

    publish_json(topic, root, 0);
}


int sensors_run(sensor_type_t type, const char* zone, const char* dev_id, int period_sec){
    int t = 0;

    if(period_sec <= 0) period_sec = 5;
    ssdp_init();
    mqtt_connect_with_lwt(dev_id);
    publish_info(zone, dev_id, type);

    for(;; ++t){
        if(type == SENSOR_CO){
            int base = 40 + (t % 121);                 /* 40..160 */
            int co   = (t/121) % 2 ? (160 - (t%121)) : base;
            if(co < 0) co = 0; if(co > 1000) co = 1000;
            publish_state_co(zone, dev_id, co);
            ssdp_alive_tick(dev_id, SENSOR_CO, t, period_sec);
        } else if(type == SENSOR_TEMP){
            double v = 35.0 + 30.0 * ((t % 40) / 39.0); /* 35..65 */
            publish_state_temp(zone, dev_id, v);
            ssdp_alive_tick(dev_id, SENSOR_TEMP, t, period_sec);
        } else {
            double vib  = (t % 20 == 10) ? 0.8 : (0.05 + 0.10 * ((t % 10) / 9.0));
            double tilt = (t % 30 == 15) ? 6.0 : (0.5 + 1.0 * ((t % 8) / 7.0));
            if(vib < 0.0) vib = 0.0; if(vib > 2.0) vib = 2.0;
            if(tilt < 0.0) tilt = 0.0; if(tilt > 30.0) tilt = 30.0;
            publish_state_vib(zone, dev_id, vib, tilt);
            ssdp_alive_tick(dev_id, SENSOR_VIB, t, period_sec);
        }

        mosquitto_loop(mq, 50, 1);
        sleep(period_sec);
    }
    return 0;
}

/* opcioni mali CLI za direktno pokretanje ovog fajla kao binara */
#ifndef BUILD_AS_LIB
static void usage(const char* prog){
    fprintf(stderr, "Usage: %s --type {co|temp|vib} --zone Z --id DEV_ID [--period S]\n", prog);
}
int main(int argc, char** argv){
    const char* zone = "zone1";
    const char* dev_id = 0;
    int period = 5;
    sensor_type_t type = SENSOR_CO;
    int i;

    for(i=1;i<argc;i++){
        if(strcmp(argv[i],"--type")==0 && i+1<argc){
            i++;
            if(strcmp(argv[i],"co")==0) type=SENSOR_CO;
            else if(strcmp(argv[i],"temp")==0) type=SENSOR_TEMP;
            else if(strcmp(argv[i],"vib")==0) type=SENSOR_VIB;
            else { usage(argv[0]); return 1; }
        } else if(strcmp(argv[i],"--zone")==0 && i+1<argc){
            zone = argv[++i];
        } else if(strcmp(argv[i],"--id")==0 && i+1<argc){
            dev_id = argv[++i];
        } else if(strcmp(argv[i],"--period")==0 && i+1<argc){
            period = atoi(argv[++i]);
        } else {
            usage(argv[0]); return 1;
        }
    }
    if(!dev_id){ usage(argv[0]); return 1; }
    return sensors_run(type, zone, dev_id, period);
}
#endif
