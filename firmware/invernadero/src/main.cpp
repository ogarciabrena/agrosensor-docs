/**
 * AgroSensor OGB · Invernadero inteligente ESP32
 * Lazo continuo (sin deep sleep): sensores + control por histéresis
 * de riego, ventilación y extractor + comandos manuales vía Supabase.
 *
 * Ver especificación: docs/07-invernadero.md
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "config.h"
#include "secrets.h"

DHT dht(PIN_DHT, DHT22);

// ---------- estado de un actuador ----------
struct Actuator {
  const char *name;
  uint8_t pin;
  bool state = false;
  bool manualMode = false;
  uint32_t manualUntil = 0;   // millis fin de anulación manual
  uint32_t onSince = 0;       // millis desde encendido
  uint32_t lastOff = 0;       // millis del último apagado (para cooldown de bomba)
};

Actuator pump      = { "pump",      PIN_RELAY_PUMP };
Actuator fan       = { "fan",       PIN_RELAY_FAN };
Actuator extractor = { "extractor", PIN_RELAY_EXTRACTOR };

uint32_t lastSensorRead = 0, lastReport = 0, lastPoll = 0;
float soil = NAN, airT = NAN, airH = NAN, lux = NAN;

// ---------- relés ----------
void relayWrite(Actuator &a, bool on, const char *cause);

void relayApply(const Actuator &a) {
  digitalWrite(a.pin, (a.state ^ RELAY_ACTIVE_LOW) ? HIGH : LOW);
}

// ---------- HTTP helpers ----------
String supabaseRequest(const char *method, const String &path, const String &body) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, String(SUPABASE_URL) + path);
  http.addHeader("apikey", SUPABASE_ANON_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_ANON_KEY);
  http.addHeader("Content-Type", "application/json");
  int code;
  if (strcmp(method, "GET") == 0)        code = http.GET();
  else if (strcmp(method, "PATCH") == 0) code = http.sendRequest("PATCH", body);
  else                                   code = http.POST(body);
  String resp = (code > 0 && code < 300) ? http.getString() : "";
  LOG("%s %s -> %d\n", method, path.c_str(), code);
  http.end();
  return resp;
}

void logActuator(const Actuator &a, const char *cause) {
  JsonDocument d;
  d["node_id"] = NODE_ID;
  d["actuator"] = a.name;
  d["state"] = a.state;
  d["cause"] = cause;
  String body;
  serializeJson(d, body);
  supabaseRequest("POST", "/rest/v1/actuator_log", body);
}

void relayWrite(Actuator &a, bool on, const char *cause) {
  if (a.state == on) return;
  a.state = on;
  if (on) a.onSince = millis();
  else    a.lastOff = millis();
  relayApply(a);
  LOG("[%s] %s (%s)\n", a.name, on ? "ON" : "OFF", cause);
  logActuator(a, cause);
}

// ---------- sensores ----------
float soilPercent(int raw) {
  float pct = 100.0f * (SOIL_DRY_ADC - raw) / (float)(SOIL_DRY_ADC - SOIL_WET_ADC);
  return constrain(pct, 0.0f, 100.0f);
}

void readSensors() {
  long sum = 0;
  for (int i = 0; i < 3; i++) { sum += analogRead(PIN_SOIL); delay(50); }
  soil = soilPercent(sum / 3);
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t) && t > -10 && t < 55) airT = t;
  if (!isnan(h) && h >= 0 && h <= 100) airH = h;
#ifdef WOKWI_SIMULATION
  lux = analogRead(PIN_LDR) * 29.3f;
#endif
}

// ---------- control automático (histéresis) ----------
void autoControl() {
  uint32_t now = millis();

  // Salir de modo manual al vencer el timer
  for (Actuator *a : { &pump, &fan, &extractor }) {
    if (a->manualMode && now >= a->manualUntil) {
      a->manualMode = false;
      LOG("[%s] fin de modo manual, regresa a auto\n", a->name);
    }
  }

  // Bomba: histéresis + timeout + cooldown
  if (!pump.manualMode && !isnan(soil)) {
    if (pump.state) {
      if (soil > RIEGO_OFF_PCT)                         relayWrite(pump, false, "auto");
      else if (now - pump.onSince > PUMP_MAX_ON_MS)     relayWrite(pump, false, "timeout");
    } else {
      bool cooldownOk = (pump.lastOff == 0) || (now - pump.lastOff > PUMP_COOLDOWN_MS);
      if (soil < RIEGO_ON_PCT && cooldownOk)            relayWrite(pump, true, "auto");
    }
  }

  // Ventilador por temperatura
  if (!fan.manualMode && !isnan(airT)) {
    if (fan.state  && airT < VENT_OFF_C) relayWrite(fan, false, "auto");
    if (!fan.state && airT > VENT_ON_C)  relayWrite(fan, true,  "auto");
  }

  // Extractor por humedad ambiente
  if (!extractor.manualMode && !isnan(airH)) {
    if (extractor.state  && airH < EXT_OFF_PCT) relayWrite(extractor, false, "auto");
    if (!extractor.state && airH > EXT_ON_PCT)  relayWrite(extractor, true,  "auto");
  }
}

// ---------- comandos manuales ----------
Actuator* byName(const char *n) {
  if (strcmp(n, "pump") == 0)      return &pump;
  if (strcmp(n, "fan") == 0)       return &fan;
  if (strcmp(n, "extractor") == 0) return &extractor;
  return nullptr;
}

void pollCommands() {
  String path = String("/rest/v1/commands?node_id=eq.") + NODE_ID +
                "&status=eq.pending&order=created_at.asc&limit=5";
  String resp = supabaseRequest("GET", path, "");
  if (resp.length() < 3) return;

  JsonDocument doc;
  if (deserializeJson(doc, resp) != DeserializationError::Ok) return;

  for (JsonObject cmd : doc.as<JsonArray>()) {
    long id = cmd["id"];
    const char *act = cmd["actuator"];
    const char *action = cmd["action"];
    long durS = cmd["duration_s"] | 3600;
    const char *createdAt = cmd["created_at"];  // expiración la valida el MCP; aquí ejecutamos FIFO
    (void)createdAt;

    Actuator *a = byName(act);
    String newStatus = "executed";
    if (a) {
      if (strcmp(action, "auto") == 0) {
        a->manualMode = false;
      } else {
        a->manualMode = true;
        a->manualUntil = millis() + (uint32_t)durS * 1000UL;
        relayWrite(*a, strcmp(action, "on") == 0, "manual");
      }
    } else {
      newStatus = "expired";
    }

    String body = String("{\"status\":\"") + newStatus + "\",\"executed_at\":\"now()\"}";
    supabaseRequest("PATCH", String("/rest/v1/commands?id=eq.") + id, body);
  }
}

// ---------- reporte de lecturas ----------
void reportReading() {
  JsonDocument d;
  d["node_id"] = NODE_ID;
  if (!isnan(soil)) d["soil_moisture"] = round(soil * 10) / 10.0;
  if (!isnan(airT)) d["air_temp"] = round(airT * 10) / 10.0;
  if (!isnan(airH)) d["air_humidity"] = round(airH * 10) / 10.0;
  if (!isnan(lux))  d["light_lux"] = round(lux);
  String body;
  serializeJson(d, body);
  supabaseRequest("POST", "/rest/v1/readings", body);
}

// ---------- main ----------
void setup() {
#if DEBUG
  Serial.begin(115200);
#endif
  // Relés en estado seguro ANTES de todo
  for (Actuator *a : { &pump, &fan, &extractor }) {
    pinMode(a->pin, OUTPUT);
    a->state = false;
    relayApply(*a);
  }

  dht.begin();
  analogSetPinAttenuation(PIN_SOIL, ADC_11db);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(300);
  LOG("\n=== Invernadero %s en línea ===\n", NODE_ID);

  for (Actuator *a : { &pump, &fan, &extractor }) logActuator(*a, "boot");
}

void loop() {
  uint32_t now = millis();

  if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();

  if (now - lastSensorRead > CONTROL_INTERVAL_MS) {
    lastSensorRead = now;
    readSensors();
    autoControl();
  }
  if (now - lastPoll > POLL_INTERVAL_MS) {
    lastPoll = now;
    pollCommands();
  }
  if (now - lastReport > REPORT_INTERVAL_MS) {
    lastReport = now;
    reportReading();
  }
  delay(100);
}
