#include "AdminRouter.h"
#include "MappingStore.h"

#include "express/middleware/JsonMiddleware.h"
#include "express/middleware/BasicAuthentication.h"
#include "express/Router.h"

#include <nlohmann/json.hpp>
using nlohmann::json;

static json mergeMapping(const json& base, const json& patch) {
  json out = base;
  if (patch.contains("connection")) out["connection"] = patch["connection"];
  if (patch.contains("mapping")) {
    if (!out.contains("mapping") || !out["mapping"].is_object()) out["mapping"] = json::object();
    for (auto& [k,v] : patch["mapping"].items()) out["mapping"][k] = v;
  }
  return out;
}

express::Router makeAdminRouter(std::shared_ptr<MappingStore> store,
                                const AdminOptions& opt) {
  express::Router api;

  api.use(express::middleware::JsonMiddleware());
  api.use(express::middleware::BasicAuthentication(opt.user, opt.pass, opt.realm));

  // POST /config/update
  api.post("/config/update", [store] APPLICATION(req, res) {
    try {
      const std::string bodyStr(req->body.begin(), req->body.end());
      json body = json::parse(bodyStr);

      json current = store->load();
      json next;
      if (body.contains("mapping") || body.contains("connection")) {
        next = mergeMapping(current, body);
      } else {
        next = body; // treat as full file
      }

      if (!next.contains("mapping")) next["mapping"] = json::object();
      store->save(next);

      res->status(200).json({{"status","updated"}, {"path", store->path()}, {"size", next.dump().size()}});
    } catch (const std::exception& e) {
      res->status(400).json({{"error", e.what()}});
    }
  });

  // POST /config/deploy (stub)
  api.post("/config/deploy", [] APPLICATION(req, res) {
    res->status(200).json({{"status","deploy-ack"}, {"note","hot-reload hook to be wired"}});
  });

  return api; // << return by value
}
