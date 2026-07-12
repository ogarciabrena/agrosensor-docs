/**
 * AgroSensor OGB · Nodo sensor ESP32
 * Lee humedad de suelo, temp/humedad ambiente y luz.
 * Envía a Supabase REST cada 5 min con deep sleep.
 * Buffer offline en NVS si no hay red.
 *
 * Compilación real:      pio run -e nodo
 * Simulación Wokwi:      pio run -e wokwi  (usa potenciómetro y LDR)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "config.h"
#include "secrets.h"

#ifndef WOKWI_SIMULATION
#include <Wire.h>
#include <BH1750.h>
BH1750 lightMeter;
#endif

DHT dht(PIN_DHT, DHT22);
Preferences prefs;

// ---------- utilidades ----------

float readAveraged(std::function<float()> reader, float minV, float maxV) {
  float sum = 0;
  int valid = 0;
  for (int i = 0; i < SAMPLES; i++) {
    float v = reader();
    if (!isnan(v) && v >= minV && v <= maxV) {
      sum += v;
      valid++;
    }
    delay(SAMPLE_DELAY_MS);
  }
  return valid > 0 ? sum / valid : NAN;
}

float soilPercent(float rawAdc) {
  if (isnan(rawAdc)) return NAN;
  float pct = 100.0f * (SOIL_DRY_ADC - rawAdc) / (float)(SOIL_DRY_ADC - SOIL_WET_ADC);
  return constrain(pct, 0.0f, 100.0f);
}

float readBatteryV() {
  // Divisor 1:2 en GPIO 35
  uint32_t mv = analogReadMilliVolts(PIN_BATTERY);
  return (mv * 2) / 1000.0f;
}

// ---------- payload ----------

String buildPayload(uint32_t offlineDelayS) {
  JsonDocument doc;
  doc["node_id"] = NODE_ID;

  float soilRaw = readAveraged([] { return (float)analogRead(PIN_SOIL); }, 500, 4095);
  float soil = soilPercent(soilRaw);
  if (!isnan(soil)) doc["soil_moisture"] = round(soil * 10) / 10.0;

  float airT = readAveraged([] { return dht.readTemperature(); }, -10, 55);
  if (!isnan(airT)) doc["air_temp"] = round(airT * 10) / 10.0;

  float airH = readAveraged([] { return dht.readHumidity(); }, 0, 100);
  if (!isnan(airH)) doc["air_humidity"] = round(airH * 10) / 10.0;

#ifdef WOKWI_SIMULATION
  // LDR analógico como proxy de luz en simulación
  float lux = readAveraged([] { return (float)analogRead(PIN_LDR); }, 0, 4095);
  if (!isnan(lux)) doc["light_lux"] = round(lux * 29.3); // escala aprox a 0-120k
#else
  float lux = readAveraged([] { return lightMeter.readLightLevel(); }, 0, 120000);
  if (!isnan(lux)) doc["light_lux"] = round(lux);
#endif

  doc["battery_v"] = round(readBatteryV() * 100) / 100.0;
  doc["offline_delay_s"] = offlineDelayS;

  String out;
  serializeJson(doc, out);
  return out;
}

// ---------- red ----------

bool wifiConnect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t0 > WIFI_TIMEOUT_MS) return false;
    delay(250);
  }
  return true;
}

int postToSupabase(const String &jsonBody) {
  WiFiClientSecure client;
  client.setInsecure(); // Supabase usa CA pública; para producción fijar el root CA
  HTTPClient http;
  http.begin(client, String(SUPABASE_URL) + "/rest/v1/readings");
  http.addHeader("apikey", SUPABASE_ANON_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_ANON_KEY);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=minimal");
  int code = http.POST(jsonBody);
  http.end();
  return code;
}

// ---------- buffer offline (NVS) ----------

void bufferStore(const String &payload) {
  prefs.begin("agro", false);
  uint16_t head = prefs.getUShort("head", 0);
  uint16_t count = prefs.getUShort("count", 0);
  prefs.putString(("r" + String(head)).c_str(), payload);
  head = (head + 1) % BUFFER_MAX;
  if (count < BUFFER_MAX) count++;
  prefs.putUShort("head", head);
  prefs.putUShort("count", count);
  prefs.end();
  LOG("Lectura guardada en buffer. Total: %d\n", count);
}

void bufferFlush() {
  prefs.begin("agro", false);
  uint16_t head = prefs.getUShort("head", 0);
  uint16_t count = prefs.getUShort("count", 0);
  if (count == 0) { prefs.end(); return; }

  uint16_t toSend = min(count, (uint16_t)BATCH_MAX);
  String batch = "[";
  for (uint16_t i = 0; i < toSend; i++) {
    uint16_t idx = (head + BUFFER_MAX - count + i) % BUFFER_MAX;
    String item = prefs.getString(("r" + String(idx)).c_str(), "");
    if (item.length() == 0) continue;
    // actualizar offline_delay_s sumando los ciclos transcurridos
    JsonDocument d;
    if (deserializeJson(d, item) == DeserializationError::Ok) {
      d["offline_delay_s"] = (uint32_t)d["offline_delay_s"] + (count - i) * SLEEP_SECONDS;
      item = "";
      serializeJson(d, item);
    }
    if (batch.length() > 1) batch += ",";
    batch += item;
  }
  batch += "]";

  int code = postToSupabase(batch);
  if (code == 201) {
    count -= toSend;
    prefs.putUShort("count", count);
    LOG("Buffer: lote de %d enviado. Restan %d\n", toSend, count);
  } else {
    LOG("Buffer: fallo el lote (HTTP %d)\n", code);
  }
  prefs.end();
}

// ---------- main ----------

void setup() {
#if DEBUG
  Serial.begin(115200);
  delay(200);
#endif
  LOG("\n=== AgroSensor %s despierta ===\n", NODE_ID);

  dht.begin();
#ifndef WOKWI_SIMULATION
  Wire.begin(PIN_SDA, PIN_SCL);
  lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE);
#endif
  analogSetPinAttenuation(PIN_SOIL, ADC_11db);
  analogSetPinAttenuation(PIN_BATTERY, ADC_11db);
  delay(1500); // estabilización DHT22

  String payload = buildPayload(0);
  LOG("Payload: %s\n", payload.c_str());

  if (wifiConnect()) {
    int code = postToSupabase(payload);
    LOG("POST HTTP %d\n", code);
    if (code == 201) {
      bufferFlush();
    } else {
      bufferStore(payload);
    }
  } else {
    LOG("Sin WiFi\n");
    bufferStore(payload);
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  LOG("Durmiendo %d s\n", SLEEP_SECONDS);
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_SECONDS * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() { /* nunca llega: deep sleep reinicia en setup() */ }
