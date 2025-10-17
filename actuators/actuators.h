#pragma once
#include <stdbool.h>
#include <mosquitto.h>

void act_init(struct mosquitto* m);
void act_set_fan(const char* zone, const char* id, bool on);
void act_set_alarm(const char* zone, const char* id, bool on);
void act_set_fire_ext(const char* zone, const char* id, bool on);
void act_set_barrier_down(const char* id, int down);
