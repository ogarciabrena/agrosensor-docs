# 01 · Arquitectura

## Visión general

Tres capas desacopladas. Cada una puede evolucionar sin tocar las otras.

```mermaid
flowchart TB
    subgraph CAPA1["1 · Capa de campo"]
        N1[Nodo ESP32 #1]
        N2[Nodo ESP32 #2]
        N3[Nodo ESP32 #N]
    end

    subgraph CAPA2["2 · Capa de datos (Supabase)"]
        API[REST API / PostgREST]
        DB[(Postgres)]
        RLS[Row Level Security]
        API --> RLS --> DB
    end

    subgraph CAPA3["3 · Capa de análisis (cualquier máquina)"]
        MCP[Servidor MCP · stdio]
        CLI[Claude Desktop / Claude Code]
        CLI <-->|MCP protocol| MCP
    end

    N1 & N2 & N3 -->|HTTPS POST cada 5 min| API
    MCP -->|consultas SQL vía supabase-js| DB
```

## Flujo de una lectura

```mermaid
sequenceDiagram
    participant E as ESP32
    participant S as Supabase REST
    participant P as Postgres
    participant M as MCP Server
    participant C as Claude

    E->>E: Despierta de deep sleep
    E->>E: Lee sensores (3 muestras, promedia)
    E->>S: POST /rest/v1/readings (anon key)
    S->>P: INSERT (validado por RLS)
    S-->>E: 201 Created
    E->>E: Deep sleep 5 min

    Note over C,M: Horas o días después, en cualquier máquina
    C->>M: "¿Cómo va la humedad del nodo 1?"
    M->>P: SELECT + regresión lineal
    M-->>C: Tendencia, horas al umbral, recomendación de riego
```

## Manejo de fallas de red (buffer offline)

```mermaid
flowchart TD
    A[Lectura de sensores] --> B{¿WiFi + POST OK?}
    B -- Sí --> C[Enviar lectura actual]
    C --> D{¿Hay lecturas<br/>en buffer NVS?}
    D -- Sí --> E[Enviar lote pendiente<br/>máx 20 por ciclo]
    D -- No --> F[Deep sleep 5 min]
    E --> F
    B -- No --> G[Guardar en NVS<br/>buffer circular máx 100]
    G --> F
    F --> A
```

## Decisiones de diseño

| Decisión | Alternativa descartada | Razón |
|---|---|---|
| MCP local por **stdio** | MCP remoto HTTP/SSE en VPS | Cero infraestructura, cero superficie de ataque, corre en cualquier máquina. El análisis no necesita estar disponible 24/7 — los datos sí, y de eso se encarga Supabase. |
| ESP32 → Supabase **directo** | Edge Function intermedia | Menos partes móviles. RLS de solo-insert acota el riesgo de la anon key expuesta en firmware. |
| **Deep sleep** 5 min | Lectura continua | La humedad del suelo cambia en horas, no en segundos. Consumo ~10 µA dormido → meses con batería. |
| Sensor **capacitivo** | Sensor resistivo | El resistivo se corroe en semanas por electrólisis. |
| Timestamps **del servidor** (`default now()`) | Reloj del ESP32 | Evita depender de NTP en campo. Con lecturas cada 5 min, el error de segundos es irrelevante. Las lecturas del buffer offline incluyen `offline_delay_s` para reconstruir el tiempo real aproximado. |

## Escalabilidad futura (fuera de alcance inicial)

- Dashboard PWA en GitHub Pages leyendo Supabase con anon key + política RLS de lectura pública.
- LoRa/ESP-NOW para nodos sin cobertura WiFi (un nodo gateway).
- Migración del MCP a VPS con transporte HTTP/SSE **solo si** se necesita acceso multi-usuario o cron de alertas 24/7 — la spec del servidor no cambia, solo el transporte.
