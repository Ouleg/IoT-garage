#pragma once
// MQTT teme
#define TOPIC_SENSOR_FMT   "garage/%s/sensors/%s"   // zone, type
#define TOPIC_CMD_FMT      "garage/%s/cmd/%s"       // zone, actuator
#define TOPIC_STATE_FMT    "garage/%s/state/%s"     // zone, actuator
#define TOPIC_STATUS_FMT   "garage/devices/%s/status" // id

// SSDP ST/NT šifre (primer)
#define ST_SENSOR_CO       "urn:garage:device:sensor:co:1"
#define ST_ACTUATOR_FAN    "urn:garage:device:actuator:fan:1"

// JSON ključevi (opis)
#define JSON_ID            "id"
#define JSON_GROUP         "group"
#define JSON_SERVICE_TEMP  "TempService"
#define JSON_PARAM_TEMP    "Temperature"
#define JSON_SERVICE_CO    "COService"
#define JSON_PARAM_CO      "COppm"
#define JSON_SERVICE_FAN   "FanService"
#define JSON_PARAM_FAN     "State"   // "ON"/"OFF"
#define JSON_PARAM_SPEED   "Speed"   // 0..3

// JSON ključevi (komanda)
#define JSON_CMD_TYPE      "command_type" // "GET" | "SET"
#define JSON_CMD_JSON      "json"         // "state" | "info"
#define JSON_CMD_DEVICE    "device"
#define JSON_CMD_GROUP     "group"
#define JSON_CMD_SERVICE   "service"
#define JSON_CMD_PARAM     "parameter"
#define JSON_CMD_VALUE     "value"
