# Invernadero inteligente · Firmware

Implementa la especificación de [`docs/07-invernadero.md`](../../docs/07-invernadero.md): control por histéresis de bomba de riego, ventilador y extractor, con anulación manual desde Claude vía la tabla `commands` de Supabase.

> ⚠️ **En simulación los LEDs representan los relés.** Nota: los LEDs encienden con lógica inversa si `RELAY_ACTIVE_LOW=true` (los módulos de relé reales activan en LOW). Para que la simulación se vea intuitiva, pon `RELAY_ACTIVE_LOW false` en `config.h` al simular.

## Simulación en Wokwi

1. Compila: `pio run -e wokwi` (o pega los archivos en wokwi.com)
2. Usa `wokwi/diagram.json`
3. Baja el potenciómetro de humedad por debajo de 35 % → enciende LED azul (bomba). Súbelo arriba de 55 % → se apaga. Calienta el DHT22 arriba de 30 °C → ventilador.

## Prueba de comandos manuales

Con el firmware corriendo (real o Wokwi), inserta en Supabase SQL Editor:

```sql
insert into commands (node_id, actuator, action, duration_s)
values ('invernadero-01', 'pump', 'on', 120);
```

En máximo 30 s la bomba enciende por 2 minutos y regresa a modo automático. El MCP hace esto mismo con la tool `send_command` cuando le dices a Claude "riega el invernadero 2 minutos".

## Hardware real

- Módulo de 4 relés con optoacoplador (activo en LOW) en GPIO 25/26/33
- Ejecutar antes `supabase/002_invernadero.sql`
- `cp include/secrets.h.example include/secrets.h`, llenar, y `pio run -e invernadero -t upload`
