#pragma once
#define MQTT_HOST          "127.0.0.1"
#define MQTT_PORT          1883
#define MQTT_KEEPALIVE     60

// Wildcard subscribe (više zona)
#define SUB_PATTERN_TEMP "garage/+/sensors/temp"
#define SUB_PATTERN_CO   "garage/+/sensors/co"
#define SUB_PATTERN_VIB  "garage/+/sensors/vibration"

#define TEMP_BAD_C     60.0
#define CO_BAD_PPM     100.0
#define VIB_BAD_G      0.60
#define TILT_BAD_DEG   5.0

// SSDP
#define SSDP_ADDR          "239.255.255.250"
#define SSDP_PORT          1900
#define SSDP_MAX_AGE       60      // sek.
#define SSDP_MX            1       // 1..5 sek (poglavlje 4.3.2)

// LOCATION može biti i fake URL; dovoljno da parsiramo USN/ST:
#define DEV_HTTP_LOCATION_FMT "http://%s:%d/desc/%s"
