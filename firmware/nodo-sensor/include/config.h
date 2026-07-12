#pragma once

// ---------- Identidad y ciclo ----------
#define NODE_ID        "nodo-01"
#define SLEEP_SECONDS  300        // 5 minutos
#define WIFI_TIMEOUT_MS 15000

// ---------- Muestreo ----------
#define SAMPLES         3
#define SAMPLE_DELAY_MS 200

// ---------- Calibración humedad suelo (medir por nodo, ver docs/calibracion) ----------
#define SOIL_DRY_ADC 3200   // sensor al aire
#define SOIL_WET_ADC 1200   // sensor en agua

// ---------- Pines ----------
#define PIN_SOIL    34   // ADC1_CH6 — capacitivo AOUT
#define PIN_DHT     27   // DHT22
#define PIN_SDA     21   // BH1750
#define PIN_SCL     22
#define PIN_BATTERY 35   // divisor 1:2
#ifdef WOKWI_SIMULATION
#define PIN_LDR     32   // fotoresistencia en simulación
#endif

// ---------- Buffer offline ----------
#define BUFFER_MAX 100
#define BATCH_MAX  20

// ---------- Debug ----------
#define DEBUG 1
#if DEBUG
#define LOG(...) Serial.printf(__VA_ARGS__)
#else
#define LOG(...)
#endif
