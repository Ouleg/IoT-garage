otvaramo prvi terminal na pi-ju i pokrecemo:

mosquitto_sub -h 192.168.1.48 -t "garage/#" -v -R

To je kontroler koji se subscribe na senzore garaze i prati stanja, kada dodje do nekog granicnog stanja on salje komandu aktuatorima da reaguju. u drugom pi prozoru pokrenemo 

./bin/iot-controller

i tu pratimo te komande koje salje kontroler sa pi-ja
sa laptop strane prvo pokrenemo terminal koji salje stanja senzora, npr za koncentraciju CO u ppm :

mosquitto_pub -h 192.168.1.48 -t "garage/zone1/sensors/co"   -m '{"id":"zone1-co-1234","group":"sensors","COService":{"COppm":"123"}}' -q 1

to se vidi u prvom terminalu od pi-ja u kojem prati stanja senzora i ako je potrebno pokrece akciju, ovde smo poslali 123 ppm sto je alarmantno i on odmah salje akciju 


[CTRL] Rule: CO high -> CMD fan ON speed=2

ona je u drugom prozoru terminala pi-ja odnosno komande aktuatora.
U novom prozoru terminala na laptopu pratimo stanja aktuatora evo npr za fan koji regulise CO, ide subscribe na pi kontroler:

mosquitto_sub -h 192.168.1.48 -t "garage/zone1/cmd/fan" -v

i on vraca :
garage/zone1/cmd/fan {"state":"ON","speed":2}

