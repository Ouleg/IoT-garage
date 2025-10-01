// controller.c
// gcc controller.c -o controller -lmosquitto -ljansson

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <mosquitto.h>
#include "config.h"
#include "protocol.h"
#include "registry.h"

static struct mosquitto* mq = NULL;

static void rule_engine_on_sensor(const char* zone, const char* type, const char* payload)
{
    if (strcmp(type,"co")==0) {
        // ultra-minimalno: ako payload sadrži broj >=80 -> pali fan speed 2
        if (strstr(payload, "\"COppm\":\"8") || strstr(payload, "\"COppm\":\"9") || strstr(payload, "\"COppm\":\"1")) {
            char topic[128]; snprintf(topic, sizeof topic, TOPIC_CMD_FMT, zone, "fan");
            const char* cmd = "{\"state\":\"ON\",\"speed\":2}";
            mosquitto_publish(mq, NULL, topic, (int)strlen(cmd), cmd, MQTT_QOS, false);
            printf("[CTRL] Rule: CO high -> CMD fan ON speed=2\n");
        }
    }
}

static void on_message(struct mosquitto* m, void* ud, const struct mosquitto_message* msg)
{
    if (!msg || !msg->topic || !msg->payloadlen) return;
    // Telemetrija: garage/<zone>/sensors/<type>
    char zone[32], type[32];
    if (sscanf(msg->topic, "garage/%31[^/]/sensors/%31[^/]", zone, type)==2) {
        rule_engine_on_sensor(zone, type, (const char*)msg->payload);
        // keš stanja opcionalno – ako payload ima "id": "...", ažuriraj registry_set_state(id, payload)
    }
    // Info: garage/<zone>/info/<id> → registry_set_info(...)
    char dev_id[64];
    if (sscanf(msg->topic, "garage/%31[^/]/info/%63s", zone, dev_id)==2) {
        registry_set_info(dev_id, (const char*)msg->payload);
    }
}

static void mqtt_init()
{
    mosquitto_lib_init();
    mq = mosquitto_new("controller-ctrl", true, NULL);
    mosquitto_message_callback_set(mq, on_message);
    if (mosquitto_connect(mq, MQTT_HOST, MQTT_PORT, MQTT_KEEPALIVE)!=MOSQ_ERR_SUCCESS)
        fprintf(stderr,"[CTRL] MQTT connect failed\n");

    // Sub na sve senzore + sve info objave
    mosquitto_subscribe(mq, NULL, "garage/+/sensors/#", MQTT_QOS);
    mosquitto_subscribe(mq, NULL, "garage/+/info/#",   MQTT_QOS);
}

int main()
{
    registry_init();
    mqtt_init();

    extern void ssdp_ctrl_tick_discover(void); // šalje M-SEARCH; prima 200 OK i upiše u registry

    for (;;) {
        mosquitto_loop(mq, 100, 1);
        ssdp_ctrl_tick_discover(); // npr. svakih 10 s
        // periodično obriši istekle uređaje:
        registry_prune_expired(time(NULL), SSDP_MAX_AGE*2);
        usleep(100*1000);
    }
    return 0;
}
