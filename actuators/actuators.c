// actuators.c
// gcc actuators.c -o actuators -lmosquitto -ljansson

#include <stdio.h>
#include <string.h>
#include <mosquitto.h>
#include "config.h"
#include "protocol.h"

static const char* DEV_ID   = "zone1-fan-5678";
static const char* Z = ZONE;
static struct mosquitto* mq = NULL;
static int fan_state = 0, fan_speed = 0;

static void on_cmd(struct mosquitto* m, void* ud, const struct mosquitto_message* msg)
{
    // očekujemo JSON: {"state":"ON","speed":2}
    if (!msg || !msg->payload) return;
    if (strstr((const char*)msg->payload, "\"ON\""))  { fan_state=1; }
    if (strstr((const char*)msg->payload, "\"OFF\"")) { fan_state=0; }
    if (strstr((const char*)msg->payload, "\"speed\":")) { fan_speed=2; } // demo

    // potvrdi stanje
    char topic[128]; snprintf(topic, sizeof topic, TOPIC_STATE_FMT, Z, "fan");
    char payload[128];
    snprintf(payload, sizeof payload,
             "{\"id\":\"%s\",\"group\":\"actuators\",\"FanService\":{\"State\":\"%s\",\"Speed\":\"%d\"}}",
              DEV_ID, fan_state?"ON":"OFF", fan_speed);
    mosquitto_publish(mq, NULL, topic, (int)strlen(payload), payload, MQTT_QOS, false);
    printf("[ACTUATOR] Fan -> %s speed=%d\n", fan_state?"ON":"OFF", fan_speed);
}

int main()
{
    extern void ssdp_dev_loop(const char* dev_id, const char* st_urn);

    mosquitto_lib_init();
    mq = mosquitto_new(DEV_ID, true, NULL);
    char status_topic[128];
    snprintf(status_topic, sizeof status_topic, TOPIC_STATUS_FMT, DEV_ID);
    mosquitto_will_set(mq, status_topic, 7, "offline", MQTT_QOS, true);

    mosquitto_message_callback_set(mq, on_cmd);

    if (mosquitto_connect(mq, MQTT_HOST, MQTT_PORT, MQTT_KEEPALIVE) == MOSQ_ERR_SUCCESS) {
        mosquitto_publish(mq, NULL, status_topic, 6, "online", MQTT_QOS, true);
        char cmd_topic[128]; snprintf(cmd_topic, sizeof cmd_topic, TOPIC_CMD_FMT, Z, "fan");
        mosquitto_subscribe(mq, NULL, cmd_topic, MQTT_QOS);
    }

    for (;;) {
        mosquitto_loop(mq, 100, 1);
        ssdp_dev_loop(DEV_ID, ST_ACTUATOR_FAN);
        usleep(100*1000);
    }
    return 0;
}
