#pragma once

// Trajanja faza (sekunde)
#define PHASE1_NORMAL_SEC   20   // sve OK
#define PHASE2_CO_RISE_SEC  25   // CO prelazi prag (ventilator ON)
#define PHASE3_TEMP_SPIKE   20   // Temp prelazi prag (alarm ON)
#define PHASE4_VIB_EVENT    15   // Tilt veći od praga (opciono alarm)
#define PHASE5_RECOVERY     25   // sve se vraća ispod praga

// Period objave senzora (sekunde)
#define PERIOD_CO_SEC       2
#define PERIOD_TEMP_SEC     3
#define PERIOD_VIB_SEC      5

// ID-jevi (da se poklope sa kontrolerom)
#define SIM_FAN_ID   "fan_1"
#define SIM_ALARM_ID "alarm_1"

// Helper makroi
#define CLAMP(v, lo, hi) ((v)<(lo)?(lo):((v)>(hi)?(hi):(v)))