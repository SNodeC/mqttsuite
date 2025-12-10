#pragma once
#include <nlohmann/json.hpp>
#include <mutex>
#include <string>
#include <functional>

class MappingStore {
public:
  explicit MappingStore(std::string path);
  
  nlohmann::json load();
  void save(const nlohmann::json& j);

  /**
   * @brief Atomically loads, modifies, and saves the configuration.
   * 
   * @param modifier A function that takes the current JSON state and modifies it in-place.
   *                 If this function throws, the file is not updated.
   */
  void modify(std::function<void(nlohmann::json&)> modifier);

  const std::string& path() const { return filePath; }

private:
  // Internal helpers that assume the mutex is already locked
  nlohmann::json loadLocked();
  void saveLocked(const nlohmann::json& j);

  std::string filePath;
  std::mutex mu;
};