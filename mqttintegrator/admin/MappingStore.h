#pragma once
#include <nlohmann/json.hpp>
#include <mutex>
#include <string>

class MappingStore {
public:
  explicit MappingStore(std::string path);
  nlohmann::json load();                  // throws on parse errors
  void save(const nlohmann::json& j);     // truncates & writes atomically-ish
  const std::string& path() const { return filePath; }

private:
  std::string filePath;
  std::mutex mu;
};
