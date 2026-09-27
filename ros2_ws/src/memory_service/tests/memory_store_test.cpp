#include "memory_store.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

struct Fixture {
  fs::path path;
  explicit Fixture(const std::string &name) : path(fs::temp_directory_path() / ("memory_service_test_" + name + ".json")) {
    fs::remove(path);
  }
  ~Fixture() { fs::remove(path); }
};

void check(bool condition, const char *reason) {
  if (!condition) throw std::runtime_error(reason);
}

void test_opt_in_is_required() {
  Fixture f("opt_in");
  memory_service::MemoryStore store(f.path);
  check(!store.add("alice", "hobby", "chess", false), "declined add must fail");
  check(!fs::exists(f.path), "declined add must not write disk");
  check(!store.get("alice", "hobby", true).has_value(), "declined add must not exist");
  check(store.add("alice", "hobby", "chess", true), "opted-in add must succeed");
  check(store.get("alice", "hobby", true) == "chess", "exact fact must be retrievable");
  check(!store.get("alice", "hobby", false).has_value(), "read also requires live consent");
  check(store.list("alice", 20, false).empty(), "list also requires live consent");
}

void test_owner_isolation_and_no_fuzzy_answer() {
  Fixture f("isolation");
  memory_service::MemoryStore store(f.path);
  check(store.add("alice", "pet", "cat", true), "alice add failed");
  check(store.add("bob", "pet", "dog", true), "bob add failed");
  check(store.get("alice", "pet", true) == "cat", "alice must get own fact");
  check(store.get("bob", "pet", true) == "dog", "bob must get own fact");
  check(!store.get("charlie", "pet", true), "unknown owner must not see facts");
  check(!store.get("alice", "pets", true), "no fuzzy or fabricated fact");
  check(!store.get("", "pet", true), "empty owner cannot access facts");
}

void test_restart_forget_and_bounded_list() {
  Fixture f("restart");
  {
    memory_service::MemoryStore store(f.path);
    check(store.add("alice", "a", "one", true), "add a failed");
    check(store.add("alice", "b", "two", true), "add b failed");
    check(store.add("bob", "secret", "other", true), "add bob failed");
  }
  {
    memory_service::MemoryStore store(f.path);
    auto rows = store.list("alice", 1, true);
    check(rows.size() == 1 && rows[0].first == "a" && rows[0].second == "one", "list must be sorted and bounded");
    check(store.list("alice", 0, true).empty(), "zero limit yields no rows");
    check(store.forget("alice"), "forget must delete existing person");
    check(!store.get("alice", "a", true), "forget must erase in memory");
    check(store.get("bob", "secret", true) == "other", "forget must preserve other owner");
  }
  memory_service::MemoryStore reloaded(f.path);
  check(!reloaded.get("alice", "a", true), "forget must survive restart");
  check(reloaded.get("bob", "secret", true) == "other", "other owner survives restart");
  check(!reloaded.forget("alice"), "missing person reports false");
}

void test_failure_preserves_memory_and_corruption_fails_closed() {
  Fixture f("failure");
  {
    memory_service::MemoryStore store(f.path);
    check(store.add("alice", "k", "v", true), "initial add failed");
    fs::create_directory(f.path.string() + ".tmp"); // should not interfere with unique temporary names
    fs::remove_all(f.path.string() + ".tmp");
  }
  {
    std::ofstream out(f.path, std::ios::trunc);
    out << "not json";
  }
  bool threw = false;
  try { memory_service::MemoryStore store(f.path); } catch (const std::exception &) { threw = true; }
  check(threw, "corrupted storage must fail closed, never silently clear it");
}

void test_stale_temporary_file_is_not_overwritten() {
  Fixture f("stale_temp");
  const fs::path stale(f.path.string() + ".1.tmp");
  {
    std::ofstream out(stale);
    out << "sentinel";
  }
  try {
    memory_service::MemoryStore store(f.path);
    check(store.add("alice", "fact", "true", true), "stale temp must not block persistence");
    std::ifstream in(stale);
    std::string original;
    in >> original;
    check(original == "sentinel", "stale temp must never be overwritten");
  } catch (...) { fs::remove(stale); throw; }
  fs::remove(stale);
}

void test_limits_and_failed_write() {
  Fixture f("limits");
  memory_service::MemoryStore store(f.path);
  check(!store.add("", "key", "value", true), "empty owner must be rejected");
  check(!store.add("alice", "", "value", true), "empty key must be rejected");
  check(!store.add("alice", "key", "", true), "empty value must be rejected");
  check(!store.add("alice", "key", std::string(2049, 'x'), true), "oversize fact must be rejected");
  for (int i = 0; i < 25; ++i)
    check(store.add("alice", "key" + std::to_string(i), "value", true), "bounded setup failed");
  check(store.list("alice", 1000000, true).size() == 20, "listing must have a hard server-side cap");
  check(store.list("alice", 2, true).size() == 2, "requested limit must be honored");
  // Simulate an unavailable destination after a successful write. No uncommitted fact may leak.
  fs::remove(f.path);
  fs::create_directory(f.path);
  bool failed = false;
  try { store.add("alice", "new", "private", true); } catch (const std::exception &) { failed = true; }
  check(failed, "failed replacement must report an error");
  check(!store.get("alice", "new", true), "failed write must not mutate in-memory state");
  fs::remove(f.path);
}

int main() {
  try {
    test_stale_temporary_file_is_not_overwritten();
    test_opt_in_is_required();
    test_owner_isolation_and_no_fuzzy_answer();
    test_restart_forget_and_bounded_list();
    test_failure_preserves_memory_and_corruption_fails_closed();
    test_limits_and_failed_write();
    std::cout << "6 memory store tests passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
}
