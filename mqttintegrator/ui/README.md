<!-- snodec:begin page-header -->
<a id="page-overview"></a>
<p><picture><source media="(max-width: 640px)" srcset="../../docs/media/nav/banner-integrator-ui-compact.svg"><img src="../../docs/media/nav/banner-integrator-ui.svg" alt="MQTTSuite — Integrator UI" width="100%"></picture></p>
<p><a href="../../docs/guides/mapping.md#page-overview" title="Up to Mapping"><picture><source media="(prefers-color-scheme: dark)" srcset="../../docs/media/nav/up-mapping-dark.svg"><img src="../../docs/media/nav/up-mapping-light.svg" alt="Up to Mapping" title="Up to Mapping" width="160" height="28"></picture></a><a href="https://github.com/SNodeC" title="SNode.C organization"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../../docs/media/nav/organization-icon-dark.svg"><source media="(max-width: 640px)" srcset="../../docs/media/nav/organization-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../../docs/media/nav/organization-dark.svg"><img align="right" src="../../docs/media/nav/organization-light.svg" alt="SNode.C organization" title="SNode.C organization"></picture></a><a href="../../README.md#project-overview" title="MQTTSuite repository"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../../docs/media/nav/repository-icon-dark.svg"><source media="(max-width: 640px)" srcset="../../docs/media/nav/repository-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../../docs/media/nav/repository-dark.svg"><img align="right" src="../../docs/media/nav/repository-light.svg" alt="MQTTSuite repository" title="MQTTSuite repository"></picture></a><br clear="all"></p>
<!-- snodec:end page-header -->

# MQTT Integrator Mapping UI

This Angular web interface edits, validates, deploys and rolls back MQTTIntegrator mappings through the admin API.

## Development server

To start a local development server, run:

```bash
ng serve
```

Once the server is running, open your browser and navigate to `http://localhost:4200/`. The application will automatically reload whenever you modify any of the source files.

## Code scaffolding

Angular CLI includes powerful code scaffolding tools. To generate a new component, run:

```bash
ng generate component component-name
```

For a complete list of available schematics (such as `components`, `directives`, or `pipes`), run:

```bash
ng generate --help
```

## Building

To build the project run:

```bash
ng build
```

This will compile your project and store the build artifacts in the `dist/` directory. By default, the production build optimizes your application for performance and speed.

## Running unit tests

To execute unit tests with the [Karma](https://karma-runner.github.io) test runner, use the following command:

```bash
ng test
```

## Running end-to-end tests

For end-to-end (e2e) testing, run:

```bash
ng e2e
```

Angular CLI does not come with an end-to-end testing framework by default. You can choose one that suits your needs.

## Additional Resources

For more information on using the Angular CLI, including detailed command references, visit the [Angular CLI Overview and Command Reference](https://angular.dev/tools/cli) page.

## MQTTSuite integration

Imported directly from the local `mqtt-integrator-ui` checkout at commit `32ec07b`, including its six working-tree modifications. The recorded origin is `git@github.com:trnkma/mqtt-integrator-ui.git`. These sources are maintained in MQTTSuite; no external checkout or submodule is required. Original source notices and attribution are retained.

Use Node.js 22 (22.12 or newer) or Node.js 24, with npm. From this directory, run `npm ci` and `npm run build` to replace `../html/`. Commit source changes, the lockfile, and generated HTML together. Ordinary C++ builds and cross builds install the checked-in HTML without requiring Node or npm.

After configuring MQTTSuite with CMake, `cmake --build <build-directory> --target mqttintegrator-ui` performs the same clean dependency installation and production build. Reconfigure CMake after regeneration so IDE asset lists reflect the new files.

The runtime option is `integrator --html-dir <directory>`, validated as an existing directory. Its default is `<install-prefix>/var/www/mqttsuite/mqttintegrator`. CMake installs the complete HTML tree in the corresponding application component. Custom directories must contain the production build, including its assets.

The admin entry point is `/ui/index.html`. Production API calls use the same origin as the interface; Angular navigation uses URL fragments so refreshes do not require server-side route fallback. Authentication remains HTTP Basic authentication on the existing admin router. For development, `npm start` uses the `/api` proxy to the integrator on port 8085.
