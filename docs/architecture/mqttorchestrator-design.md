# MQTT Orchestrator Design (MQTTSuite / SNode.C)

## 1) Goal

`mqttorchestrator` is a MQTTSuite application for low-code IoT orchestration with a node-graph mindset similar to Node-RED, but implemented in native C++ with the existing SNode.C networking and HTTP stack.

Primary objectives:

- Provide a JSON-driven flow model that can replace mapper-only transformations.
- Support runtime flow execution (not only static design-time metadata).
- Expose a rich block catalog reusable by frontend and external tooling.
- Offer REST-first scenario lifecycle: create, validate, execute, inspect telemetry.

## 2) Canonical JSON model

All blocks share these descriptor objects:

- `inputs[]`: typed input ports.
- `outputs[]`: typed output ports.
- `telemetry[]`: runtime counters/gauges/state metrics.

Scenario model:

- `id`: unique scenario id.
- `nodes[]`: `{id, type, config}`.
- `routes[]`: `{fromNode, fromOutput, toNode, toInput}`.

Execution request model:

- `inputs[]`: `{nodeId, input, value}` initial injection set.

Execution response model:

- `steps`, `trace[]`, `outputs[]`, `telemetry` snapshot.

## 3) Rich building block set (Node-RED style categories)

Implemented categories include:

1. **Connectors**
   - `mqtt.source`, `mqtt.sink`
2. **Logic**
   - `logic.threshold`, `logic.and`, `logic.or`, `logic.not`, `logic.switch`
3. **Transform**
   - `transform.scale`, `transform.concat`, `transform.jsonpath`
4. **Time / Control**
   - `time.schedule`, `system.delay`, `system.rate-limit`
5. **Storage**
   - `storage.memory`
6. **Notification**
   - `notify.webhook`, `notify.email`
7. **Home automation**
   - `home.light`, `home.thermostat`, `home.blind`, `home.lock`, `home.scene`
8. **Sensors**
   - `sensor.temperature`, `sensor.humidity`

This gives a significantly broader starting catalog for IoT orchestration compared with mapper-centric flows.

## 4) Runtime flow processing design

The engine now executes flows in iterative graph passes:

1. Validate and load scenario nodes/routes.
2. Inject initial execution inputs into node input buffers.
3. Evaluate eligible nodes (`evaluateNode`) and produce output map.
4. Route node outputs to downstream node inputs according to `routes[]`.
5. Repeat until no more route-driven changes or max iteration cap is reached.
6. Return trace, outputs, and per-node telemetry.

Notes:

- The current execution model is deterministic and synchronous per request.
- Telemetry is persisted in-memory per scenario and node.
- Runtime state map exists for future blocks requiring memory (RBE/window/sequence).

## 5) REST API

Base path: `/api/orchestrator`

- `GET /blocks`: block catalog + shared schema.
- `GET /scenarios`: list scenarios.
- `GET /scenarios/:id`: get one scenario.
- `POST /scenarios`: create/update scenario.
- `DELETE /scenarios/:id`: delete scenario.
- `POST /scenarios/:id/execute`: execute flow with injected inputs.
- `GET /scenarios/:id/telemetry`: runtime telemetry snapshot.

## 6) Frontend app design

The included frontend is framework-free and intentionally small:

- Lists all available blocks.
- Creates a demo home-automation scenario.
- Sends execution payload to `/execute`.
- Fetches and displays telemetry via `/telemetry`.

This keeps alignment with the “no extra framework” constraint and MQTTSuite style.

## 7) Extended general review: SNode.C and MQTTSuite

### 7.1 Architectural strengths

1. **Transport breadth with coherent abstractions**
   - IPv4/IPv6/Unix and TLS variants are integrated under consistent idioms.
2. **C++ express-like server ergonomics**
   - JSON middleware and router API accelerate API app development.
3. **Composition-friendly component model**
   - App-level executables can share common patterns while keeping binary boundaries.
4. **Performance-oriented native implementation**
   - Suitable for edge gateways with tight resource budgets.
5. **Operational awareness**
   - Logging hooks, reconnect/retry behaviors, and clear broker/bridge/integrator responsibilities.

### 7.2 Weak spots and friction points

1. **Steep learning curve**
   - Heavy template and transport combinatorics can slow onboarding.
2. **Cross-app boilerplate repetition**
   - CORS/error middleware and static hosting patterns are duplicated.
3. **Domain schema lifecycle**
   - As catalogs grow (orchestrator blocks), schema governance/versioning becomes critical.
4. **State persistence strategy**
   - Several runtime models are memory-first; durable state/options should be standardized.
5. **Frontend complexity trajectory**
   - Single-file HTML can become hard to maintain as graph editing matures.

### 7.3 Recommended roadmap

1. Add shared API utilities (`common/http`) for consistent CORS/errors/validation.
2. Introduce versioned schema registry + schema tests in CI.
3. Add pluggable persistence layer for scenarios and runtime states.
4. Add runtime scheduler/event loop integration for periodic/time-based blocks.
5. Add graph compiler passes:
   - topological sort,
   - cycle strategy declarations,
   - static type checks across routes.
6. Add compatibility adapter from existing mapfile entries to generated flow scenarios.

### 7.4 Hands-on validation notes (April 2026)

Based on a full upstream build/install verification run (SNode.C + MQTTSuite):

1. **Dependency hygiene matters for reproducible builds**
   - `lib/json-schema-validator` must be present via submodules (`git submodule update --init --recursive`).
   - `nlohmann_json` CMake package discovery must be available in the active prefix.
2. **SNode.C install path consistency is good**
   - `snodecConfig.cmake` and component targets install cleanly under `/usr/local/lib/cmake/snodec`.
   - MQTTSuite consumers can resolve core transport/http/mqtt components without hand-written include/link flags.
3. **Current practical gap: optional transport components**
   - `net-l2-*` / `net-rc-*` components are optional and may not be available unless bluetooth dependencies are installed.
   - Bridge build scripts should treat these as optional features and avoid hard failures when unavailable.
4. **Orchestrator integration quality gate**
   - The orchestrator subproject should resolve package dependencies before subdirectory graph traversal to avoid imported-target ordering issues.
   - This is now addressed by resolving `find_package(nlohmann_json ...)` before `add_subdirectory(lib)`.

Net conclusion: the platform architecture is solid and production-oriented, but the build/dependency ergonomics should be tightened to reduce first-build friction for contributors and CI runners.

## 8) Migration path replacing mapper behavior

1. Parse legacy mapfile items into equivalent node blocks (`mqtt.source` + transform + `mqtt.sink`).
2. Auto-generate scenario JSON for existing deployments.
3. Dual-run mode:
   - mapper output and orchestrator output both available for comparison.
4. Promote orchestrator to primary runtime once parity is confirmed.
