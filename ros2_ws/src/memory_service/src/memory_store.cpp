#include "memory_store.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <system_error>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace memory_service {
namespace {
std::atomic<unsigned long long> serial{0};
bool valid_id(const std::string &s) { return !s.empty() && s.size() <= 128; }
bool valid_key(const std::string &s) { return !s.empty() && s.size() <= 128; }

void write_all(const std::filesystem::path &file, const std::string &bytes) {
#ifdef _WIN32
  int fd = -1;
  const int open_result = _wsopen_s(&fd, file.c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY,
                                   _SH_DENYRW, _S_IREAD | _S_IWRITE);
  if (open_result != 0) throw std::system_error(open_result, std::generic_category(), "open temporary memory file");
  try {
    std::size_t offset = 0;
    while (offset < bytes.size()) {
      int count = _write(fd, bytes.data() + offset,
                         static_cast<unsigned int>(std::min<std::size_t>(bytes.size() - offset, 1 << 20)));
      if (count <= 0) throw std::system_error(errno, std::generic_category(), "write temporary memory file");
      offset += static_cast<std::size_t>(count);
    }
    if (_commit(fd) != 0) throw std::system_error(errno, std::generic_category(), "sync temporary memory file");
  } catch (...) { _close(fd); throw; }
  if (_close(fd) != 0) throw std::system_error(errno, std::generic_category(), "close temporary memory file");
#else
  int fd = ::open(file.c_str(), O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (fd == -1) throw std::system_error(errno, std::generic_category(), "open temporary memory file");
  try {
    std::size_t offset = 0;
    while (offset < bytes.size()) {
      ssize_t count = ::write(fd, bytes.data() + offset, bytes.size() - offset);
      if (count < 0 && errno == EINTR) continue;
      if (count <= 0) throw std::system_error(errno, std::generic_category(), "write temporary memory file");
      offset += static_cast<std::size_t>(count);
    }
    if (::fsync(fd) != 0) throw std::system_error(errno, std::generic_category(), "sync temporary memory file");
  } catch (...) { ::close(fd); throw; }
  if (::close(fd) != 0) throw std::system_error(errno, std::generic_category(), "close temporary memory file");
#endif
}
}

MemoryStore::MemoryStore(std::filesystem::path path) : path_(std::move(path)) {
  if (path_.empty()) throw std::invalid_argument("storage path is empty");
  if (!std::filesystem::exists(path_)) return;
  if (std::filesystem::file_size(path_) > 8 * 1024 * 1024)
    throw std::runtime_error("memory file exceeds maximum size");
  std::ifstream input(path_, std::ios::binary);
  if (!input) throw std::runtime_error("cannot read memory file");
  nlohmann::json parsed = nlohmann::json::parse(input); // Corruption fails closed.
  if (!parsed.is_object() || !parsed.contains("version") || parsed.at("version") != 1 ||
      !parsed.contains("people") || !parsed.at("people").is_object())
    throw std::runtime_error("unsupported memory format; legacy unscoped data is not imported");
  for (auto it = parsed.at("people").begin(); it != parsed.at("people").end(); ++it) {
    if (!valid_id(it.key()) || !it.value().is_object() || it.value().size() > max_facts_per_person)
      throw std::runtime_error("invalid person memory data");
    auto &facts = people_[it.key()];
    for (auto fact = it.value().begin(); fact != it.value().end(); ++fact) {
      if (!valid_key(fact.key()) || !fact.value().is_string() || fact.value().get<std::string>().size() > 2048)
        throw std::runtime_error("invalid memory fact");
      facts.emplace(fact.key(), fact.value().get<std::string>());
    }
  }
}

void MemoryStore::persist(const People &candidate) {
  nlohmann::json people = nlohmann::json::object();
  for (const auto &[person, facts] : candidate) people[person] = facts;
  const std::string bytes = nlohmann::json{{"version", 1}, {"people", people}}.dump(2);
  std::filesystem::path temp;
  do {
    temp = std::filesystem::path(path_.string() + "." + std::to_string(++serial) + ".tmp");
  } while (std::filesystem::exists(temp));
  try {
    write_all(temp, bytes);
#ifdef _WIN32
    if (!MoveFileExW(temp.c_str(), path_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
      throw std::system_error(GetLastError(), std::system_category(), "replace memory file");
#else
    std::filesystem::rename(temp, path_);
    const auto dir = path_.has_parent_path() ? path_.parent_path() : std::filesystem::path(".");
    int dir_fd = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY);
    if (dir_fd == -1) throw std::system_error(errno, std::generic_category(), "open memory directory");
    const int result = ::fsync(dir_fd);
    const int saved_errno = errno;
    ::close(dir_fd);
    if (result != 0) throw std::system_error(saved_errno, std::generic_category(), "sync memory directory");
#endif
  } catch (...) {
    std::error_code ignored;
    std::filesystem::remove(temp, ignored);
    throw;
  }
}

bool MemoryStore::add(const std::string &person_id, const std::string &key,
                      const std::string &value, bool consent) {
  if (!consent || !valid_id(person_id) || !valid_key(key) || value.empty() || value.size() > 2048) return false;
  std::lock_guard<std::mutex> lock(mutex_);
  People candidate = people_;
  auto &facts = candidate[person_id];
  if (!facts.count(key) && facts.size() >= max_facts_per_person) return false;
  facts[key] = value;
  persist(candidate); // Never expose an uncommitted fact when disk write fails.
  people_.swap(candidate);
  return true;
}

std::optional<std::string> MemoryStore::get(const std::string &person_id,
                                            const std::string &key, bool consent) const {
  if (!consent || !valid_id(person_id) || !valid_key(key)) return std::nullopt;
  std::lock_guard<std::mutex> lock(mutex_);
  const auto owner = people_.find(person_id);
  if (owner == people_.end()) return std::nullopt;
  const auto fact = owner->second.find(key);
  if (fact == owner->second.end()) return std::nullopt;
  return fact->second;
}

std::vector<std::pair<std::string, std::string>> MemoryStore::list(
    const std::string &person_id, std::size_t limit, bool consent) const {
  std::vector<std::pair<std::string, std::string>> results;
  if (!consent || !valid_id(person_id) || !limit) return results;
  std::lock_guard<std::mutex> lock(mutex_);
  const auto owner = people_.find(person_id);
  if (owner == people_.end()) return results;
  for (const auto &fact : owner->second) {
    if (results.size() >= std::min(limit, max_list)) break;
    results.push_back(fact);
  }
  return results;
}

bool MemoryStore::forget(const std::string &person_id) {
  if (!valid_id(person_id)) return false;
  std::lock_guard<std::mutex> lock(mutex_);
  if (!people_.count(person_id)) return false;
  People candidate = people_;
  candidate.erase(person_id);
  persist(candidate);
  people_.swap(candidate);
  return true;
}
} // namespace memory_service
