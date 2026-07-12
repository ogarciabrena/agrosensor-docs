# Nodo sensor · Firmware

Implementa los requisitos F1–F10 de [`docs/03-firmware.md`](../../docs/03-firmware.md).

## Hardware real

1. Cablear según [`docs/02-hardware.md`](../../docs/02-hardware.md)
2. `cp include/secrets.h.example include/secrets.h` y llenar credenciales
3. Calibrar el sensor: anotar ADC en aire (`SOIL_DRY_ADC`) y en agua (`SOIL_WET_ADC`) en `include/config.h`
4. `pio run -e nodo -t upload && pio device monitor`

## Simulación (sin hardware) — Wokwi

Opción A · **wokwi.com** (más fácil):
1. Nuevo proyecto ESP32 → pegar `src/main.cpp`, `include/config.h` y crear `secrets.h` con SSID `Wokwi-GUEST` y pass vacío
2. Pegar `wokwi/diagram.json` en la pestaña diagram.json
3. Agregar en Library Manager: DHT sensor library, ArduinoJson
4. Play ▶ — mueve el potenciómetro (humedad) y el LDR (luz) y verás los POST llegar a Supabase real

Opción B · VS Code + extensión Wokwi:
```bash
pio run -e wokwi
# luego F1 -> "Wokwi: Start Simulator" (usa wokwi/wokwi.toml)
```

> En simulación el sensor de luz es un LDR analógico (GPIO 32) en lugar del BH1750, activado por el flag `WOKWI_SIMULATION`. Wokwi tiene salida real a internet, así que la simulación inserta lecturas en tu Supabase de verdad.
