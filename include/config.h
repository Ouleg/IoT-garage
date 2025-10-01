#pragma once
#define MQTT_HOST          "127.0.0.1"
#define MQTT_PORT          1883
#define MQTT_KEEPALIVE     30
#define MQTT_QOS           1

// Primer zone i ID-eva
#define ZONE               "zone1"

// SSDP
#define SSDP_ADDR          "239.255.255.250"
#define SSDP_PORT          1900
#define SSDP_MAX_AGE       60      // sek.
#define SSDP_MX            1       // 1..5 sek (poglavlje 4.3.2)

// LOCATION može biti i fake URL; dovoljno da parsiramo USN/ST:
#define DEV_HTTP_LOCATION_FMT "http://%s:%d/desc/%s"
