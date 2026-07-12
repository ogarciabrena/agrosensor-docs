# 🌱 AgroSensor OGB — Agricultura de Precisión

Sistema de agricultura de precisión basado en **ESP32 + Supabase + MCP**, diseñado para monitoreo de cultivos en Oaxaca (agave, hortalizas, milpa).

> **Este repositorio contiene la especificación completa, diagramas, y el firmware ESP32 con simulación Wokwi.** El prompt de `docs/06` permite que **Claude Code** genere el servidor MCP en cualquier máquina. El servidor MCP corre **localmente vía stdio** en cualquier equipo (laptop, PC, VPS opcional) — no requiere infraestructura dedicada.

## Arquitectura en un vistazo

```mermaid
flowchart LR
    subgraph Campo
        S1[Sensor humedad suelo]
        S2[BME280 temp/humedad]
        S3[BH1750 luz]
        E[ESP32<br/>deep sleep 5 min]
        S1 --> E
        S2 --> E
        S3 --> E
    end

    E -- "HTTPS POST<br/>REST API" --> SB[(Supabase<br/>Postgres + RLS)]

    subgraph "Cualquier máquina"
        MCP[Servidor MCP<br/>Node.js · stdio]
        C[Claude Desktop /<br/>Claude Code]
        C <--> MCP
    end

    MCP -- "service role key" --> SB
```

## Contenido

| Documento | Descripción |
|---|---|
| [docs/01-arquitectura.md](docs/01-arquitectura.md) | Arquitectura general, flujo de datos, decisiones de diseño |
| [docs/02-hardware.md](docs/02-hardware.md) | BOM, diagrama de conexiones, alimentación solar |
| [docs/03-firmware.md](docs/03-firmware.md) | Especificación del firmware ESP32 (PlatformIO) |
| [docs/04-supabase.md](docs/04-supabase.md) | Esquema SQL, políticas RLS, seguridad de llaves |
| [docs/05-mcp-server.md](docs/05-mcp-server.md) | Servidor MCP portable (stdio) y sus 7 herramientas de análisis |
| [docs/06-prompt-claude-code.md](docs/06-prompt-claude-code.md) | Prompt listo para copiar en Claude Code |
| [docs/07-invernadero.md](docs/07-invernadero.md) | 🆕 Invernadero inteligente: control de riego/ventilación/extractor |
| [firmware/nodo-sensor/](firmware/nodo-sensor/) | 🆕 Firmware del nodo de campo (deep sleep + buffer offline) + simulación Wokwi |
| [firmware/invernadero/](firmware/invernadero/) | 🆕 Firmware del invernadero (histéresis + comandos manuales) + simulación Wokwi |
| [supabase/001_schema.sql](supabase/001_schema.sql) | Migración SQL base |
| [supabase/002_invernadero.sql](supabase/002_invernadero.sql) | 🆕 Migración: comandos manuales y log de actuadores |

## Filosofía del proyecto

1. **Costo cero de infraestructura**: Supabase free tier + MCP local. Nada que mantener 24/7.
2. **Portable**: el MCP corre en cualquier máquina con Node.js 20+. Se conecta a Claude Desktop o Claude Code con 4 líneas de configuración.
3. **Seguro por diseño**: el ESP32 solo puede *insertar* lecturas (RLS). La llave `service_role` nunca sale de la máquina de análisis.
4. **Documentación primero**: todo el sistema está especificado aquí antes de escribir una línea de código. Claude Code implementa a partir de esta spec.

## Quick start

```bash
# 1. Crea el proyecto en Supabase y ejecuta supabase/001_schema.sql
# 2. En cualquier máquina con Node.js y Claude Code:
claude
# 3. Pega el prompt de docs/06-prompt-claude-code.md
```

## Licencia

MIT — Omar García Brena · OGB Consulting
