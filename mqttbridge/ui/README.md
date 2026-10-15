<a id="banner"></a>

<!-- snodec:begin page-header -->
<a id="banner"></a><a id="page-overview"></a>
<p><picture><source media="(max-width: 640px)" srcset="../../docs/media/nav/banner-bridge-ui-compact.svg"><img src="../../docs/media/nav/banner-bridge-ui.svg" alt="MQTTSuite — Bridge UI" width="100%"></picture></p>
<p><a href="../../docs/guides/bridging.md#banner" title="Up to Bridging"><picture><source media="(prefers-color-scheme: dark)" srcset="../../docs/media/nav/up-bridging-dark.svg"><img src="../../docs/media/nav/up-bridging-light.svg" alt="Up to Bridging" title="Up to Bridging" width="160" height="28"></picture></a><a href="https://github.com/VolkerChristian#banner" title="Volker Christian account"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../../docs/media/nav/account-icon-dark.svg"><source media="(max-width: 640px)" srcset="../../docs/media/nav/account-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../../docs/media/nav/account-dark.svg"><img align="right" src="../../docs/media/nav/account-light.svg" alt="Volker Christian account" title="Volker Christian account"></picture></a><a href="https://github.com/SNodeC#banner" title="SNode.C organization"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../../docs/media/nav/organization-icon-dark.svg"><source media="(max-width: 640px)" srcset="../../docs/media/nav/organization-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../../docs/media/nav/organization-dark.svg"><img align="right" src="../../docs/media/nav/organization-light.svg" alt="SNode.C organization" title="SNode.C organization"></picture></a><a href="../../README.md#banner" title="MQTTSuite repository"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../../docs/media/nav/repository-icon-dark.svg"><source media="(max-width: 640px)" srcset="../../docs/media/nav/repository-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../../docs/media/nav/repository-dark.svg"><img align="right" src="../../docs/media/nav/repository-light.svg" alt="MQTTSuite repository" title="MQTTSuite repository"></picture></a><br clear="all"></p>
<!-- snodec:end page-header -->

# MQTT Bridge Configuration UI

This is a code bundle for a graphical interface for the MQTT Bridge Configuration in the MQTTSuite. It is based on a Figma prototype available at https://www.figma.com/design/YdgbzQUl5u5550Kv88Jw7G/MQTT-Bridge-Configuration-UI-improved.

## Running the code

Run `npm i` to install the dependencies.

Run `npm run dev` to start the development server.


## MQTTSuite integration

Imported directly from SNodeC/mqtt-bridge-config-ui at commit `47303745064e693f71909f00bc67a01d4884c98f`. These sources are maintained in MQTTSuite; no external checkout or submodule is required. Original source notices and attribution are retained.

Use Node.js 22 (22.12 or newer) or Node.js 24, with npm. From this directory, run `npm ci` and `npm run build` to replace `../html/`. Commit source changes, the lockfile, and generated HTML together. Ordinary C++ builds and cross builds install the checked-in HTML without requiring Node or npm.

After configuring MQTTSuite with CMake, `cmake --build <build-directory> --target mqttbridge-ui` performs the same clean dependency installation and production build. Reconfigure CMake after regeneration so IDE asset lists reflect the new files.

The runtime option is `bridge --html-dir <directory>`, validated as an existing directory. Its default is `<install-prefix>/var/www/mqttsuite/mqttbridge`. CMake installs the complete HTML tree in the corresponding application component. Custom directories must contain the production build, including its assets.

The interface is served at `/config/index.html`; its API and event stream remain at `/api/bridge/*`. The broker client dashboard is independent of these sources.
