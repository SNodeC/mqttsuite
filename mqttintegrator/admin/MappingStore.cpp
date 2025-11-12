#include "MappingStore.h"
#include <fstream>

MappingStore::MappingStore(std::string path) : filePath(std::move(path)) {}

nlohmann::json MappingStore::load() {
  std::lock_guard<std::mutex> lk(mu);
  std::ifstream f(filePath);
  if (!f) throw std::runtime_error("Cannot open mapping file: " + filePath);
  nlohmann::json j; f >> j;
  return j;
}

void MappingStore::save(const nlohmann::json& j) {
  std::lock_guard<std::mutex> lk(mu);
  // write to a temp file then rename for basic atomicity
  const std::string tmp = filePath + ".tmp";
  {
    std::ofstream out(tmp, std::ios::trunc);
    if (!out) throw std::runtime_error("Cannot open temp mapping file for write: " + tmp);
    out << j.dump(2) << std::endl;
  }
  std::rename(tmp.c_str(), filePath.c_str());
}
