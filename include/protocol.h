#pragma once

// MQTT teme
#define TOPIC_SENSOR_FMT   "garage/%s/sensors/%s"   // zone, type
#define TOPIC_CMD_FMT      "garage/%s/cmd/%s"       // zone, actuator
#define TOPIC_STATUS_FMT   "garage/devices/%s/status" // id

// JSON ključevi za cJSON
#define JSON_ID              "id"
#define JSON_GROUP           "group" // sensors ili actuators

#define JSON_SERVICE_CO      "COService"
#define JSON_PARAM_CO        "COppm"

#define JSON_SERVICE_TEMP    "TempService"
#define JSON_PARAM_TEMP      "temperature"

#define JSON_SERVICE_VIB     "VibService"
