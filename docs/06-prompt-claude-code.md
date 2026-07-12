# 06 · Prompt para Claude Code

Copiar el bloque completo como primer mensaje en Claude Code, ejecutándolo **desde la raíz de este repositorio clonado** (así puede leer toda la documentación local).

---

```
Lee toda la documentación de este repositorio (README.md, docs/01 a 05 y
supabase/001_schema.sql). Es la especificación completa del proyecto AgroSensor.
Implementa lo siguiente en dos carpetas hermanas a este repo:

## 1. ../agro-analisis — Capa de análisis agnóstica (prioridad 1)

- Node.js 20+, TypeScript estricto, @supabase/supabase-js.
- ARQUITECTURA OBLIGATORIA (docs/05-capa-analisis.md): núcleo puro en
  src/core/ que implementa la interfaz AgroCore completa (11 funciones,
  incluyendo las 3 del invernadero del doc 07) con las reglas de análisis
  del final del doc 05. El core NO importa nada de protocolos.
- Tres adaptadores delgados en src/adapters/:
  - mcp.ts: @modelcontextprotocol/sdk con StdioServerTransport, una tool
    por función del core.
  - rest.ts: Fastify, un endpoint por función, auth x-api-key, y
    /openapi.json autogenerado.
  - cli.ts: comandos básicos (trend, anomalies, riega, status, export).
- Variables de entorno vía dotenv: SUPABASE_URL, SUPABASE_SERVICE_ROLE_KEY,
  API_KEY (solo para el adaptador REST).
- Errores: el core lanza errores tipados; cada ADAPTADOR los captura y
  los traduce a su protocolo. Nunca tirar el proceso.
- Tests unitarios del core en test/core/ (vitest) con el repo mockeado:
  regresión, z-score, histéresis de deduplicación de alertas.
- Scripts npm: build, mcp, rest, cli, test.
- README en español: instalación, registro del adaptador MCP en Claude
  Desktop/Code, uso del REST con cualquier LLM vía OpenAPI, ejemplos CLI.

## 2. ../agro-esp32 — Firmware (prioridad 2)

NOTA: el firmware de referencia ya existe en firmware/ de este repo
(nodo-sensor e invernadero). Úsalo como base, revísalo, corrige lo que
encuentres y complétalo — no lo reescribas desde cero.

- PlatformIO, board esp32dev, framework Arduino.
- Implementa TODOS los requisitos F1–F10 de docs/03-firmware.md: deep sleep
  5 min, 3 muestras promediadas con validación de rangos, calibración por
  nodo, POST a Supabase REST, buffer offline en NVS (Preferences) circular
  de 100 con envío por lotes de 20 y campo offline_delay_s.
- include/secrets.h.example y include/config.h según el doc 03.
- .gitignore que excluya include/secrets.h.
- docs/calibracion.md con el procedimiento seco/húmedo paso a paso.
- README en español: cableado (referenciar docs/02-hardware.md de este repo),
  compilación y flasheo con PlatformIO.

## Reglas generales

- No inventes funcionalidad que no esté en la spec. Si algo es ambiguo,
  resuélvelo con la opción más simple y déjalo anotado en el README.
- Sin dependencias innecesarias.
- Al terminar cada carpeta: inicializa git, haz commit inicial con mensaje
  descriptivo. NO hagas push — yo creo los repos remotos y hago push.
- Verifica que agro-analisis compile (npm run build) y pase los tests antes de terminar.
```

---

## Después de que Claude Code termine

1. Crear los repos vacíos en GitHub: `agro-analisis` y `agro-esp32`
2. En cada carpeta: `git remote add origin git@github.com:ogarciabrena/<repo>.git && git push -u origin main`
3. Crear el proyecto en Supabase → SQL Editor → pegar `supabase/001_schema.sql`
4. Copiar `.env.example` → `.env` en agro-analisis con las llaves reales
5. `npm install && npm run build && npm test` en agro-analisis
6. Registrar el adaptador MCP en Claude Desktop o Claude Code (comandos en docs/05) — o levantar el REST si usarás otro LLM
7. Flashear el primer nodo con PlatformIO y verificar que lleguen lecturas a Supabase
