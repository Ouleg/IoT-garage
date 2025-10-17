#pragma once

// Kontroler na RPi-ju (broker isto na RPi) → localhost:
#define MQTT_HOST          "127.0.0.1"
#define MQTT_PORT          1883
#define MQTT_KEEPALIVE     60

//Default zona
#define DEFAULT_ZONE "zone1"

#define TEMP_THRESHOLD_C     65.0
#define CO_THRESHOLD_PPM     150.0
#define TILT_THRESHOLD_DEG   10.0
#define BARRIER_HOLD_SEC     10

// Heartbeat: ako nema svežih merenja duže od…
#define SENSOR_STALE_SEC       15
