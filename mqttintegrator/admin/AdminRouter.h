#pragma once
#include "express/Router.h"
#include <memory>

class MappingStore;

struct AdminOptions {
  std::string user{"admin"};
  std::string pass{"admin"};
  std::string realm{"mqttsuite-admin"};
};

// Creates and returns a Router that handles /config/* endpoints.
express::Router makeAdminRouter(std::shared_ptr<MappingStore> store,
                                const AdminOptions& opt);
