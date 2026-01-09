#pragma once
#include "express/Router.h"
#include <string>
#include <functional>

namespace mqtt::lib::admin {

    struct AdminOptions {
        std::string user{"admin"};
        std::string pass{"admin"};
        std::string realm{"mqttsuite-admin"};
    };

    // Callback to trigger reload in the main application
    using ReloadCallback = std::function<void()>;

    // Creates and returns a Router that handles /config/* endpoints.
    express::Router makeMappingAdminRouter(const std::string& mappingFilePath,
                                           const AdminOptions& opt,
                                           ReloadCallback onDeploy);

}
