#include "MappingStore.h"
#include <fstream>
#include <cstdio>
#include <cerrno>
#include <cstring>

MappingStore::MappingStore(std::string path) : filePath(std::move(path)) {}

nlohmann::json MappingStore::load() {
  std::lock_guard<std::mutex> lk(mu);
  return loadLocked();
}

void MappingStore::save(const nlohmann::json& j) {
  std::lock_guard<std::mutex> lk(mu);
  saveLocked(j);
}

void MappingStore::modify(std::function<void(nlohmann::json&)> modifier) {
  std::lock_guard<std::mutex> lk(mu);
  nlohmann::json current = loadLocked();
  
  modifier(current);
  
  saveLocked(current);
}

nlohmann::json MappingStore::loadLocked() {
  std::ifstream f(filePath);
  if (!f) throw std::runtime_error("Cannot open mapping file: " + filePath);
  nlohmann::json j;
  f >> j;
  return j;
}

void MappingStore::saveLocked(const nlohmann::json& j) {
  // write to a temp file then rename for basic atomicity
  const std::string tmp = filePath + ".tmp";
  {
    std::ofstream out(tmp, std::ios::trunc);
    if (!out) throw std::runtime_error("Cannot open temp mapping file for write: " + tmp);
    out << j.dump(4) << std::endl; // Pretty print with 4 spaces
  }
  if (std::rename(tmp.c_str(), filePath.c_str()) != 0) {
    std::string err = std::strerror(errno);
    std::remove(tmp.c_str());
    throw std::runtime_error("Failed to rename temp file: " + err);
  }
}