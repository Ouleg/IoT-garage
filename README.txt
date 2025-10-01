proverimo da li je aktivan broker 

systemctl status mosquitto --no-pager

otvaramo prvi terminal na pi-ju i pokrecemo:

mosquitto_sub -h 192.168.1.48 -t "garage/#" -v -R

To je prikaz koji se subscribe na senzore garaze i prati stanja, kada dodje do nekog granicnog stanja on salje komandu aktuatorima da reaguju. u drugom pi prozoru pokrenemo kontroler:
  
  cc -O2 -Wall -Iinclude -Icontroller -Iactuators -Isensors -I/usr/include/cjson \
  -DBUILD_AS_LIB \
  -o garage_ctrl \
  apps/cli.c controller/controller.c controller/registry.c \
  actuators/actuators.c sensors/sensors.c ssdp_dev.c \
  $(pkg-config --cflags --libs libmosquitto) -lcjson

./garage_ctrl

ispise MQTT connected rc=0, kontroler je aktivan,
sa laptop strane prvo pokrenemo terminal koji salje stanja senzora, npr za koncentraciju CO u ppm :

mosquitto_pub -h 192.168.1.48 -t "garage/zone1/sensors/co" -m '{"id":"co01","group":"sensors","COppm":150}'

vibracija:
mosquitto_pub -h 192.168.1.48 -t "garage/zone1/sensors/vibration" \
  -m '{"id":"v01","group":"sensors","vib_g":0.9,"tilt_deg":12}'
  
temperatura:
mosquitto_pub -h 192.168.1.48 -t "garage/zone1/sensors/temp" \
  -m '{"id":"t01","group":"sensors","temperature":85}'

to se vidi u prvom terminalu od pi-ja u kojem prati stanja senzora i ako je potrebno pokrece akciju, ovde smo poslali 123 ppm sto je alarmantno i on odmah salje akciju 

pratimo state na

mosquitto_sub -h 192.168.1.48 -t "garage/+/state/#" -v
odakle vidimo stanja aktuatora da li su on, off

pratimo komande koje aktuator salje :

mosquitto_sub -h 192.168.1.48 -t "garage/+/cmd/#" -v

i on vraca :
garage/zone1/cmd/fan {"id":"fan_1","state":"ON"}

