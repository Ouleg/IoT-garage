// sensors.c
// gcc sensors.c -o sensors -lmosquitto -ljansson

#include <stdio.h>
#include <string.h>
#include <mosquitto.h>
#include "config.h"
#include "protocol.h"

// Pretpostavimo CO senzor, id "zone1-co-1234"
static const char* DEV_ID   = "zone1-co-1234";
static const char* DEV_TYPE = "sensor-co";  // samo za registry
static const char* Z = ZONE;
static struct mosquitto* mq = NULL;

static void mqtt_connect()
{
    mosquitto_lib_init();
    mq = mosquitto_new(DEV_ID, true, NULL);
    // LWT
    char status_topic[128];
    snprintf(status_topic, sizeof status_topic, TOPIC_STATUS_FMT, DEV_ID);
    mosquitto_will_set(mq, status_topic, 7, "offline", MQTT_QOS, true);

    if (mosquitto_connect(mq, MQTT_HOST, MQTT_PORT, MQTT_KEEPALIVE) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "MQTT connect failed\n");
    } else {
        mosquitto_publish(mq, NULL, status_topic, 6, "online", MQTT_QOS, true);
    }
}

static void publish_info()
{
    // minimalni opis – pogl. 4.4
    const char* info =
      "{"
      "\"id\":\"zone1-co-1234\","
      "\"group\":\"sensors\","
      "\"COService\":{\"COppm\":\"0~1000|r\"}"
      "}";
    char topic[128];
    snprintf(topic, sizeof topic, "garage/%s/info/%s", Z, DEV_ID);
    mosquitto_publish(mq, NULL, topic, (int)strlen(info), info, MQTT_QOS, true);
}

static void publish_state(int co_ppm)
{
    char payload[128];
    snprintf(payload, sizeof payload,
             "{\"id\":\"%s\",\"group\":\"sensors\",\"COService\":{\"COppm\":\"%d\"}}",
             DEV_ID, co_ppm);

    char topic[128];
    snprintf(topic, sizeof topic, TOPIC_SENSOR_FMT, Z, "co");
    mosquitto_publish(mq, NULL, topic, (int)strlen(payload), payload, MQTT_QOS, false);
}

int main()
{
    // 1) SSDP init + NOTIFY ssdp:alive (vidi ssdp_dev.c)
    extern void ssdp_dev_loop(const char* dev_id, const char* st_urn);
    // 2) MQTT
    mqtt_connect();
    publish_info();

    // 3) Glavna petlja: telemetrija svakih ~5 s
    for (int t=0;;++t) {
        int co = 40 + (t % 70); // “fake” očitavanje
        publish_state(co);
        mosquitto_loop(mq, 100, 1);
        // svake ~30s ssdp_dev_loop obnavlja NOTIFY alive i sluša M-SEARCH
        ssdp_dev_loop(DEV_ID, ST_SENSOR_CO);
        usleep(5*1000*1000);
    }
    return 0;
}


    