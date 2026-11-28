# Repository-owned package builds

Schema 2 declares the stable releases built by this CI tree and their SNode.C requirement. The native hooks build the tagged source and use the frozen `DEPENDENCIES_JSON` selections to record exact dependencies in package metadata.

The OpenWrt registration hook installs the recipe from `CI_DIR` into the shared SDK before feed resolution. Each recipe builds the tagged `SOURCE_DIR`, including its submodules. MQTTSuite uses the SNode.C development files in the SDK staging directory. The recipe's `EXTRA_DEPENDS` records the bound SNode.C version/revision in both IPK and APK metadata.

Before CMake configures, each recipe checks the SDK feed’s nlohmann_json version using pkg-config and stages the upstream CMake files in its package build directory. The imported target uses the SDK’s headers.
