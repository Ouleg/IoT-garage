// apps/cli.c (skraćeno; samo formira JSON iz linije komande)
#include <stdio.h>
#include <string.h>
#include <mosquitto.h>
#include "config.h"
#include "protocol.h"

int main()
{
    struct mosquitto* mq;
    mosquitto_lib_init();
    mq = mosquitto_new("cli-app", true, NULL);
    mosquitto_connect(mq, MQTT_HOST, MQTT_PORT, MQTT_KEEPALIVE);

    printf("Unesi komandu (npr. GET *.state | GET zone1-co-1234.info | "
           "SET zone1-fan-5678.FanService.State ON)\n> ");

    char line[256];
    while (fgets(line, sizeof line, stdin)) {
        char json[512]={0};
        if (!strncmp(line,"GET ",4)) {
            char dev[64], what[16];
            if (sscanf(line, "GET %63[^.].%15s", dev, what)==2) {
                snprintf(json,sizeof json,
                    "{\"%s\":\"GET\",\"%s\":\"%s\",\"%s\":\"%s\"}",
                    JSON_CMD_TYPE, JSON_CMD_JSON, (!strcmp(what,"info")?"info":"state"),
                    JSON_CMD_DEVICE, dev);
            } else if (strstr(line,"GET *.state")) {
                snprintf(json,sizeof json,
                    "{\"%s\":\"GET\",\"%s\":\"state\",\"%s\":\"*\"}",
                    JSON_CMD_TYPE, JSON_CMD_JSON, JSON_CMD_DEVICE);
            } else if (strstr(line,"GET *.info")) {
                snprintf(json,sizeof json,
                    "{\"%s\":\"GET\",\"%s\":\"info\",\"%s\":\"*\"}",
                    JSON_CMD_TYPE, JSON_CMD_JSON, JSON_CMD_DEVICE);
            }
        } else if (!strncmp(line,"SET ",4)) {
            char dev[64], svc[64], param[64], val[64];
            // SET devId.service.param value
            if (sscanf(line,"SET %63[^.].%63[^.].%63s %63s", dev, svc, param, val)==4) {
                snprintf(json,sizeof json,
                    "{\"%s\":\"SET\",\"%s\":\"\",\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\"}",
                    JSON_CMD_TYPE, JSON_CMD_GROUP, JSON_CMD_DEVICE, dev,
                    JSON_CMD_SERVICE, svc, JSON_CMD_PARAM, param, JSON_CMD_VALUE, val);
            }
        }
        if (json[0]) {
            mosquitto_publish(mq,NULL,"controller/req",(int)strlen(json),json,1,false);
            printf(">> %s\n", json);
        }
        printf("> ");
        mosquitto_loop(mq, 10, 1);
    }
    return 0;
}
