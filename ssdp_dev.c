#include "ssdp_dev.h"
#include <stdio.h>
#include <time.h>
#include <math.h>

static const char* nz(const char* s){ return s ? s : "-"; }

void ssdp_init(void) {}
void ssdp_deinit(void) {}

void ssdp_announce_alive(const char* st_urn, const char* usn, const char* location){
    time_t now = time(NULL);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(stderr, "[%s] SSDP NOTIFY: ST=%s USN=%s LOC=%s\n",
            ts, nz(st_urn), nz(usn), nz(location));
}

void ssdp_alive_tick(const char* dev_id, const char* st_urn, double value, int period_sec){
    (void)period_sec;
    time_t now = time(NULL);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
    if (isnan(value)){
        fprintf(stderr, "[%s] SSDP ALIVE: dev=%s st=%s\n", ts, nz(dev_id), nz(st_urn));
    } else {
        fprintf(stderr, "[%s] SSDP ALIVE: dev=%s st=%s val=%.3f\n",
                ts, nz(dev_id), nz(st_urn), value);
    }
}
