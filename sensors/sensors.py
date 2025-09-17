#!/usr/bin/env python3
import os, time, json, random, argparse
import paho.mqtt.client as mqtt

def main():
    ap = argparse.ArgumentParser(description="Garage sensors publisher")
    ap.add_argument("--host", default=os.getenv("MQTT_HOST","localhost"))
    ap.add_argument("--zone", default=os.getenv("GARAGE_ZONE","zone1"))
    ap.add_argument("--period", type=float, default=3.0, help="interval slanja u sekundama")
    ap.add_argument("--once", action="store_true", help="pošalji samo jedan set merenja i izađi")
    args = ap.parse_args()

    c = mqtt.Client(client_id=f"sensors_{args.zone}")
    c.connect(args.host, 1883, 60)

    def publish_one():
        co = random.randint(20, 120)              # ppm
        temp = random.randint(20, 70)             # °C
        vib = round(random.uniform(0.0, 0.6), 2)  # g

        c.publish(f"garage/{args.zone}/sensors/co",    json.dumps({"ppm": co}), qos=0, retain=False)
        c.publish(f"garage/{args.zone}/sensors/temp",  json.dumps({"c": temp}), qos=0, retain=False)
        c.publish(f"garage/{args.zone}/sensors/vibration", json.dumps({"g": vib}), qos=0, retain=False)

        print(f"[SENS] co={co}ppm  temp={temp}C  vib={vib}g")

    if args.once:
        publish_one()
        return

    while True:
        publish_one()
        c.loop(timeout=0.1)
        time.sleep(args.period)

if __name__ == "__main__":
    main()
