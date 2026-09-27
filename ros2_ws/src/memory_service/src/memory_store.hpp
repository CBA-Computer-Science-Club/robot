#pragma once
#include <cstddef>
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace memory_service {
// A caller-provided consent flag is not an authentication or consent record.
// The ROS caller must authenticate the subject and obtain current opt-in.
class MemoryStore {
public:
  static constexpr std::size_t max_list = 20;
  static constexpr std::size_t max_facts_per_person = 128;
  explicit MemoryStore(std::filesystem::path path);
  bool add(const std::string &person_id, const std::string &key,
           const std::string &value, bool consent);
  std::optional<std::string> get(const std::string &person_id,
                                 const std::string &key, bool consent) const;
  std::vector<std::pair<std::string, std::string>> list(
      const std::string &person_id, std::size_t limit, bool consent) const;
  bool forget(const std::string &person_id);

private:
  using People = std::map<std::string, std::map<std::string, std::string>>;
  void persist(const People &candidate);
  std::filesystem::path path_;
  mutable std::mutex mutex_;
  People people_;
};
} // namespace memory_service
