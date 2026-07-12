-- ============================================================
-- AgroSensor OGB · Esquema inicial
-- Ejecutar en: Supabase Dashboard → SQL Editor → New query
-- ============================================================

-- Nodos registrados (con calibración por sensor)
create table public.nodes (
  node_id text primary key,
  name text not null,
  location text,
  crop text,
  soil_dry_adc int default 3200,
  soil_wet_adc int default 1200,
  created_at timestamptz default now()
);

-- Lecturas de sensores
create table public.readings (
  id bigint generated always as identity primary key,
  node_id text references public.nodes(node_id),
  ts timestamptz default now(),
  soil_moisture numeric,
  soil_temp numeric,
  air_temp numeric,
  air_humidity numeric,
  light_lux numeric,
  battery_v numeric,
  offline_delay_s int default 0
);

create index idx_readings_node_ts on public.readings (node_id, ts desc);

-- Alertas generadas por el análisis del MCP
create table public.alerts (
  id bigint generated always as identity primary key,
  node_id text references public.nodes(node_id),
  ts timestamptz default now(),
  level text check (level in ('info','warning','critical')),
  type text,          -- 'low_moisture' | 'frost_risk' | 'sensor_anomaly' | 'low_battery' | 'node_offline'
  message text,
  acknowledged boolean default false
);

-- ============================================================
-- Row Level Security
-- anon key (firmware ESP32): SOLO insertar lecturas
-- service_role key (MCP local): acceso total (bypasea RLS)
-- ============================================================

alter table public.nodes enable row level security;
alter table public.readings enable row level security;
alter table public.alerts enable row level security;

create policy "esp32_insert_readings" on public.readings
  for insert to anon with check (true);

-- Nota: service_role bypasea RLS por diseño; no requiere políticas.
-- No se crea NINGUNA política de select/update/delete para anon:
-- por defecto RLS niega todo lo no permitido explícitamente.

-- ============================================================
-- Nodo de ejemplo (editar calibración tras medir seco/húmedo)
-- ============================================================

insert into public.nodes (node_id, name, location, crop, soil_dry_adc, soil_wet_adc)
values ('nodo-01', 'Nodo de pruebas', 'Miahuatlán, Oaxaca', 'hortalizas', 3200, 1200);
