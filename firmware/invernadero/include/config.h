#pragma once

#define NODE_ID "invernadero-01"

// ---------- Intervalos (ms) ----------
#define CONTROL_INTERVAL_MS 10000   // lazo de control cada 10 s
#define POLL_INTERVAL_MS    30000   // comandos manuales cada 30 s
#define REPORT_INTERVAL_MS  60000   // reporte a Supabase cada 60 s

// ---------- Umbrales con histéresis ----------
#define RIEGO_ON_PCT  35.0
#define RIEGO_OFF_PCT 55.0
#define VENT_ON_C     30.0
#define VENT_OFF_C    27.0
#define EXT_ON_PCT    85.0
#define EXT_OFF_PCT   75.0

// ---------- Protecciones bomba ----------
#define PUMP_MAX_ON_MS   (15UL * 60UL * 1000UL)  // máx 15 min continuos
#define PUMP_COOLDOWN_MS (30UL * 60UL * 1000UL)  // 30 min entre riegos

// ---------- Calibración suelo ----------
#define SOIL_DRY_ADC 3200
#define SOIL_WET_ADC 1200

// ---------- Pines ----------
#define PIN_SOIL            34
#define PIN_DHT             27
#define PIN_RELAY_PUMP      25
#define PIN_RELAY_FAN       26
#define PIN_RELAY_EXTRACTOR 33
#define RELAY_ACTIVE_LOW    true   // módulos típicos de 4 relés activan en LOW
#ifdef WOKWI_SIMULATION
#define PIN_LDR 32
#endif

#define DEBUG 1
#if DEBUG
#define LOG(...) Serial.printf(__VA_ARGS__)
#else
#define LOG(...)
#endif
