#pragma once
// MQTT teme
#define TOPIC_SENSOR_FMT   "garage/%s/sensors/%s"   // zone, type
#define TOPIC_CMD_FMT      "garage/%s/cmd/%s"       // zone, actuator
#define TOPIC_STATE_FMT    "garage/%s/state/%s"     // zone, actuator
#define TOPIC_STATUS_FMT   "garage/devices/%s/status" // id

// SSDP ST/NT šifre

// ACTUATORS
#define ST_ACTUATOR_FAN      "urn:garage:device:actuator:fan:1"
#define ST_ACTUATOR_ALARM    "urn:garage:device:actuator:alarm:1"
#define ST_ACTUATOR_BARRIER  "urn:garage:device:actuator:barrier:1"
#define ST_ACTUATOR_FIRE_EXT "urn:garage:device:actuator:fireext:1"

// SENSORS
#define ST_SENSOR_TEMP       "urn:garage:device:sensor:temp:1"
#define ST_SENSOR_CO         "urn:garage:device:sensor:co:1"
#define ST_SENSOR_VIB        "urn:garage:device:sensor:vib:1"
#define ST_SENSOR_TILT       "urn:garage:device:sensor:tilt:1"

// CONTROLLER
#define ST_CONTROLLER        "urn:garage:device:controller:1"

// JSON ključevi za cJSON
#define JSON_ID              "id"
#define JSON_GROUP           "group"

#define JSON_SERVICE_CO      "co"
#define JSON_PARAM_CO        "co_ppm"

#define JSON_SERVICE_TEMP    "temperature"
#define JSON_PARAM_TEMP      "t_c"

#define JSON_SERVICE_VIB     "vibration"
#define JSON_PARAM_V         "v"
