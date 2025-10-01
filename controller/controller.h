#pragma once
#include <mosquitto.h>

int  controller_init(struct mosquitto* m);
void controller_start_loop(struct mosquitto* m);
