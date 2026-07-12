# 04 · Supabase — Base de datos

## Modelo de datos

```mermaid
erDiagram
    NODES ||--o{ READINGS : genera
    NODES ||--o{ ALERTS : genera

    NODES {
        text node_id PK
        text name
        text location
        text crop
        int soil_dry_adc
        int soil_wet_adc
        timestamptz created_at
    }

    READINGS {
        bigint id PK
        text node_id FK
        timestamptz ts
        numeric soil_moisture
        numeric soil_temp
        numeric air_temp
        numeric air_humidity
        numeric light_lux
        numeric battery_v
        int offline_delay_s
    }

    ALERTS {
        bigint id PK
        text node_id FK
        timestamptz ts
        text level
        text type
        text message
        boolean acknowledged
    }
```

El SQL completo está en [`supabase/001_schema.sql`](../supabase/001_schema.sql) — se ejecuta una vez en el SQL Editor de Supabase.

## Modelo de seguridad (RLS)

```mermaid
flowchart TD
    subgraph Llaves
        AK[anon key<br/>vive en el firmware ESP32]
        SK[service_role key<br/>vive SOLO en la máquina del MCP]
    end

    AK -->|"INSERT en readings ✅<br/>SELECT ❌ · UPDATE ❌ · DELETE ❌<br/>acceso a nodes/alerts ❌"| DB[(Postgres)]
    SK -->|"acceso total<br/>(bypasea RLS)"| DB
```

**Por qué esto es seguro aunque la anon key esté en el firmware:**

- Lo peor que puede hacer alguien que extraiga la anon key del ESP32 es *insertar lecturas falsas*. No puede leer datos, ni borrarlos, ni tocar otras tablas.
- La `service_role` key nunca se embebe en hardware ni se sube a git — vive en un `.env` en la máquina donde corre el MCP.
- Si la anon key se compromete y hay spam de inserts: rotar la llave en Supabase y reflashear los nodos. Con pocos nodos es un procedimiento de minutos.

## Free tier de Supabase — ¿alcanza?

| Recurso | Límite free | Consumo estimado |
|---|---|---|
| Base de datos | 500 MB | 1 nodo cada 5 min ≈ 105k lecturas/año ≈ **~15 MB/año**. Alcanza para 10+ nodos por años |
| API requests | Ilimitadas (fair use) | 288 POST/día por nodo — trivial |
| Pausa por inactividad | Proyecto se pausa tras 7 días sin actividad | **No aplica**: los nodos hacen requests constantes |

## Mantenimiento

- **Retención:** opcionalmente, agregar un job (pg_cron, disponible en Supabase) que agregue lecturas > 6 meses a promedios horarios y borre el detalle. No necesario al inicio.
- **Backup:** free tier no incluye backups automáticos → el MCP incluye la herramienta `export_data` para dumps periódicos a CSV local.
