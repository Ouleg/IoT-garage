#!/usr/bin/env python3
import json, time
import paho.mqtt.client as mqtt

BROKER = "localhost"
TOPICS = [
    ("garage/zone1/sensors/co", 0),
    ("garage/zone1/sensors/temp", 0),
    ("garage/zone1/sensors/vibration", 0),
]

# pragovi (po potrebi promeni)
CO_WARN = 50       # ppm
TEMP_FIRE = 55     # °C
VIB_LIMIT = 0.3    # g

def publish_cmd(client, topic, payload_dict):
    client.publish(topic, json.dumps(payload_dict), qos=0, retain=False)
    print(f"[CMD] {topic} {payload_dict}")

def on_connect(client, userdata, flags, rc):
    print("Connected with rc=", rc)
    for t,q in TOPICS:
        client.subscribe(t)
        print("Subscribed:", t)

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode("utf-8"))
    except Exception as e:
        print("Bad JSON on", msg.topic, "->", msg.payload)
        return

    if msg.topic.endswith("/co"):
        ppm = float(data.get("ppm", 0))
        print(f"[CO] {ppm} ppm")
        if ppm > CO_WARN:
            publish_cmd(client, "garage/zone1/cmd/fan", {"id":"fan_1","state":"ON"})
        else:
            publish_cmd(client, "garage/zone1/cmd/fan", {"id":"fan_1","state":"OFF"})

    elif msg.topic.endswith("/temp"):
        c = float(data.get("c", 0))
        print(f"[TEMP] {c} C")
        if c > TEMP_FIRE:
            publish_cmd(client, "garage/zone1/cmd/alarm", {"id":"alarm_1","state":"ON"})
            publish_cmd(client, "garage/entry/cmd/barrier", {"id":"bar_1","state":"DOWN"})
        else:
            publish_cmd(client, "garage/zone1/cmd/alarm", {"id":"alarm_1","state":"OFF"})

    elif msg.topic.endswith("/vibration"):
        g = float(data.get("g", 0))
        print(f"[VIB] {g} g")
        if g > VIB_LIMIT:
            publish_cmd(client, "garage/entry/cmd/barrier", {"id":"bar_1","state":"DOWN"})

client = mqtt.Client(client_id="controller_pi")
client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER, 1883, 60)
client.loop_forever()
