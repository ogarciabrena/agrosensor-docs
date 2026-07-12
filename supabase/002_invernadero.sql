-- ============================================================
-- AgroSensor OGB · 002 Invernadero inteligente
-- Ejecutar DESPUÉS de 001_schema.sql
-- ============================================================

-- Cola de comandos manuales (Claude/MCP -> ESP32)
create table public.commands (
  id bigint generated always as identity primary key,
  node_id text references public.nodes(node_id),
  created_at timestamptz default now(),
  actuator text check (actuator in ('pump','fan','extractor')),
  action text check (action in ('on','off','auto')),
  duration_s int,                 -- null = 3600 default en firmware
  status text default 'pending' check (status in ('pending','executed','expired')),
  executed_at timestamptz
);
create index idx_commands_pending on public.commands (node_id, status) where status = 'pending';

-- Historial de actuadores
create table public.actuator_log (
  id bigint generated always as identity primary key,
  node_id text references public.nodes(node_id),
  ts timestamptz default now(),
  actuator text,
  state boolean,                  -- true = encendido
  cause text check (cause in ('auto','manual','timeout','boot'))
);
create index idx_actlog_node_ts on public.actuator_log (node_id, ts desc);

-- Vista de estado consolidado
create or replace view public.greenhouse_status as
select
  n.node_id, n.name, n.crop,
  r.ts as last_reading_ts,
  r.soil_moisture, r.air_temp, r.air_humidity, r.light_lux,
  (select jsonb_object_agg(a.actuator, jsonb_build_object('state', a.state, 'cause', a.cause, 'since', a.ts))
     from (select distinct on (actuator) actuator, state, cause, ts
             from public.actuator_log al
            where al.node_id = n.node_id
            order by actuator, ts desc) a
  ) as actuators
from public.nodes n
left join lateral (
  select * from public.readings rd
   where rd.node_id = n.node_id
   order by ts desc limit 1
) r on true;

-- ============================================================
-- RLS: el ESP32 del invernadero (anon key) puede:
--   leer y actualizar SOLO commands, e insertar en actuator_log
-- ============================================================

alter table public.commands enable row level security;
alter table public.actuator_log enable row level security;

create policy "esp32_read_commands" on public.commands
  for select to anon using (true);

create policy "esp32_ack_commands" on public.commands
  for update to anon
  using (status = 'pending')
  with check (status in ('executed','expired'));

create policy "esp32_insert_actlog" on public.actuator_log
  for insert to anon with check (true);

-- Nodo invernadero de ejemplo
insert into public.nodes (node_id, name, location, crop, soil_dry_adc, soil_wet_adc)
values ('invernadero-01', 'Invernadero principal', 'Miahuatlán, Oaxaca', 'hortalizas', 3200, 1200);
