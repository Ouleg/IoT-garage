#pragma once

void ssdp_init(void);
void ssdp_deinit(void);

// šalje NOTIFY/ALIVE (stub: loguje); koristi ST_* iz protocol.h
void ssdp_announce_alive(const char* st_urn, const char* usn, const char* location);

// periodični alive sa vrednošću (double); ako nema vrednosti, pošalji NaN
void ssdp_alive_tick(const char* dev_id, const char* st_urn, double value, int period_sec);
