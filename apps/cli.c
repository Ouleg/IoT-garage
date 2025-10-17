#include <stdio.h>
#include <mosquitto.h>
#include "../include/config.h"
#include "../include/ssdp_dev.h"
#include "../controller/controller.h"

int main(void){

    const char *host = "localhost";
    const char *zone = "zone1";
    int port = 1883;

    /* parse argv */
    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i], "--host") && i+1<argc) host = argv[++i];
        else if (!strcmp(argv[i], "--port") && i+1<argc) port = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--zone") && i+1<argc) zone = argv[++i];
    }

    /* posle connect-a: */
    char topic[128];
    snprintf(topic, sizeof(topic), "garage/%s/sensors/#", zone);
    mosquitto_subscribe(mosq, NULL, topic, 0);


    // SSDP announce (vidljiv kao kontroler)
    ssdp_init();
    char loc[256];
    snprintf(loc, sizeof(loc), DEV_HTTP_LOCATION_FMT, "raspberrypi.local", 8080, "controller.xml");
    ssdp_announce_alive("urn:garage:device:controller:1",
                        "uuid:garage-controller-1",
                        loc);

    // MQTT
    mosquitto_lib_init();
    struct mosquitto* m = mosquitto_new("garage_controller_pi", true, NULL);
    if(!m){ fprintf(stderr,"mosquitto_new failed\n"); return 1; }
    if(mosquitto_connect(m, MQTT_HOST, MQTT_PORT, MQTT_KEEPALIVE) != MOSQ_ERR_SUCCESS){
        fprintf(stderr,"MQTT connect failed\n"); return 2;
    }

    controller_init(m);
    controller_start_loop(m);

    mosquitto_destroy(m);
    mosquitto_lib_cleanup();
    return 0;
}
