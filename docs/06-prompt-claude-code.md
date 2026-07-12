# 06 · Prompt para Claude Code

Copiar el bloque completo como primer mensaje en Claude Code, ejecutándolo **desde la raíz de este repositorio clonado** (así puede leer toda la documentación local).

---

```
Lee toda la documentación de este repositorio (README.md, docs/01 a 05 y
supabase/001_schema.sql). Es la especificación completa del proyecto AgroSensor.
Implementa lo siguiente en dos carpetas hermanas a este repo:

## 1. ../agro-mcp — Servidor MCP portable (prioridad 1)

- Node.js 20+, TypeScript estricto, @modelcontextprotocol/sdk con
  StdioServerTransport, @supabase/supabase-js.
- Implementa las 8 herramientas de docs/05-mcp-server.md EXACTAMENTE como
  están especificadas, incluyendo las reglas de análisis (regresión lineal,
  z-score, sensores congelados, gaps, deduplicación de alertas, umbrales
  por cultivo, Hargreaves simplificado).
- Estructura de archivos según la sección "Estructura del repo" del doc 05.
- Variables de entorno vía dotenv: SUPABASE_URL, SUPABASE_SERVICE_ROLE_KEY.
- Cada tool con try/catch: un error de query devuelve texto explicativo,
  nunca tira el proceso.
- Scripts npm: build (tsc), start (node dist/index.js), dev (tsx src/index.ts).
- README en español con: instalación, configuración para Claude Desktop
  y Claude Code (copiar de docs/05), y ejemplos de preguntas en lenguaje
  natural que activan cada tool.

## 2. ../agro-esp32 — Firmware (prioridad 2)

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
- Verifica que agro-mcp compile (npm run build) sin errores antes de terminar.
```

---

## Después de que Claude Code termine

1. Crear los repos vacíos en GitHub: `agro-mcp` y `agro-esp32`
2. En cada carpeta: `git remote add origin git@github.com:ogarciabrena/<repo>.git && git push -u origin main`
3. Crear el proyecto en Supabase → SQL Editor → pegar `supabase/001_schema.sql`
4. Copiar `.env.example` → `.env` en agro-mcp con las llaves reales
5. `npm install && npm run build` en agro-mcp
6. Registrar el MCP en Claude Desktop o Claude Code (comandos en docs/05)
7. Flashear el primer nodo con PlatformIO y verificar que lleguen lecturas a Supabase
