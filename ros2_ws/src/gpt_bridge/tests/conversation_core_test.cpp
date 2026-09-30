#include "conversation_core.hpp"
#ifdef NDEBUG
#error "conversation_core_test requires enabled assertions"
#endif
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace gpt_bridge;

int main() {
  {
    const std::vector<std::string> memories{"likes tea\n\"Earl Grey\"", "prefers short answers"};
    const auto body = build_request("local-model", "Say \"hi\"\nnow", memories, 1, 20);
    const auto json = nlohmann::json::parse(body);
    assert(json.at("model") == "local-model");
    assert(json.at("messages").at(0).at("role") == "system");
    assert(json.at("messages").at(1).at("role") == "user");
    assert(json.at("messages").at(1).at("content") == "Say \"hi\"\nnow");
    assert(json.at("messages").at(0).at("content").get<std::string>().find("likes tea") != std::string::npos);
    assert(json.at("messages").at(0).at("content").get<std::string>().find("prefers short answers") == std::string::npos);
    assert(body.size() < 4096);
  }
  {
    const auto reply = parse_reply(R"({"choices":[{"message":{"content":"Hello \"friend\"!"}}]})");
    assert(reply == "Hello \"friend\"!");
    bool rejected = false;
    try { parse_reply(R"({"choices":[]})"); } catch (const std::exception &) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { parse_reply(R"({"choices":[{"message":{"content":null}}]})"); } catch (const std::exception &) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { parse_reply("not JSON"); } catch (const std::exception &) { rejected = true; }
    assert(rejected);
  }
  {
    const auto cmd = parse_remember("Remember: I prefer Earl Grey", 180);
    assert(cmd.has_value() && *cmd == "I prefer Earl Grey");
    assert(!parse_remember("I remember the rain", 180));
    assert(!parse_remember("remember:    ", 180));
    assert(!parse_remember("Remember: long and private", 5));
  }
  {
    assert(parse_identity("person_123") == "person_123");
    assert(parse_identity(" {\"person_id\":\"person_123\"} ") == "person_123");
    assert(parse_identity("person/id with spaces").empty());
    assert(parse_identity(R"({"person_id":"../../admin"})").empty());
    assert(parse_identity(R"({"person_id":""})").empty());
  }
  {
    const auto facts = read_facts(R"(["short fact","a much longer fact","third"])", 2, 10);
    assert(facts.size() == 2 && facts[0] == "a much lon" && facts[1] == "third");
    assert(read_facts("broken json", 3, 100).empty());
    const auto saved = append_fact(R"(["old","old"])", "new", 2, 10);
    assert(nlohmann::json::parse(saved) == nlohmann::json::array({"old", "new"}));
    std::string accumulated;
    for (int i = 0; i < 20; ++i)
      accumulated = append_fact(accumulated, std::string(180, static_cast<char>('a' + i)), 20, 180);
    assert(accumulated.size() <= 2048);  // memory_service value limit
  }
  {
    const auto request = nlohmann::json::parse(build_provider_request(
        "anthropic", "claude-model", "Say \"hello\"\nplease", {"likes tea", "not included"}, 1, 20, 256, false));
    assert(request.at("model") == "claude-model");
    assert(request.at("max_tokens") == 256);
    assert(request.at("stream") == false);
    assert(request.contains("system"));
    assert(request.at("system").get<std::string>().find("likes tea") == std::string::npos);
    assert(request.at("messages").size() == 1);
    assert(request.at("messages")[0]["role"] == "user");
    assert(request.at("messages")[0]["content"] == "Say \"hello\"\nplease");
    const auto consented = nlohmann::json::parse(build_provider_request(
        "anthropic", "claude-model", "hello", {"likes tea", "not included"}, 1, 20, 256, true));
    assert(consented.at("system").get<std::string>().find("likes tea") != std::string::npos);
    assert(consented.at("system").get<std::string>().find("not included") == std::string::npos);
    assert(build_provider_request("local", "local-model", "hi", {"tea"}, 1, 20, 256, false) ==
           build_request("local-model", "hi", {"tea"}, 1, 20));
    bool rejected = false;
    try { build_provider_request("unknown", "model", "hi", {}, 1, 20, 256, false); }
    catch (const std::exception &) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { build_provider_request("anthropic", "model", "hi", {}, 1, 20, 0, false); }
    catch (const std::exception &) { rejected = true; }
    assert(rejected);
  }
  {
    assert(parse_provider_reply("anthropic", R"({"type":"message","content":[{"type":"thinking","thinking":"private"},{"type":"text","text":"Hello"},{"type":"text","text":"friend!"}]})") == "Hello\nfriend!");
    assert(parse_provider_reply("local", R"({"choices":[{"message":{"content":"hi"}}]})") == "hi");
    for (const auto & bad : {"not json", R"({"type":"error","error":{"message":"private error"}})",
          R"({"content":[]})", R"({"content":[{"type":"text","text":null}]})",
          R"({"content":[{"type":"tool_use","name":"move"}]})"}) {
      bool rejected = false;
      try { parse_provider_reply("anthropic", bad); }
      catch (const std::exception &) { rejected = true; }
      assert(rejected);
    }
  }
  {
    assert(provider_endpoint("local", "", false) == "http://127.0.0.1:11434/v1/chat/completions");
    assert(provider_endpoint("anthropic", "", true) == "https://api.anthropic.com/v1/messages");
    assert(provider_endpoint("local", "http://localhost:8080/v1/chat/completions", false) == "http://localhost:8080/v1/chat/completions");
    for (const auto & url : {"http://api.anthropic.com/v1/messages", "https://api.anthropic.com.evil/v1/messages",
         "https://api.anthropic.com/v1/messages?key=secret", "http://localhost:99999/a", "http://otherhost:80/a"}) {
      bool rejected = false;
      try { provider_endpoint("anthropic", url, true); }
      catch (const std::exception &) { rejected = true; }
      assert(rejected);
    }
    for (const auto & provider : {"anthropic", "unknown"}) {
      bool rejected = false;
      try { provider_endpoint(provider, "", false); }
      catch (const std::exception &) { rejected = true; }
      assert(rejected);
    }
    assert(provider_headers("local", "ignored-key", "").size() == 1);
    const auto headers = provider_headers("anthropic", "unit-test-key", "wrkspc_unit_test");
    assert(headers.size() == 4);
    assert(headers[1] == "Authorization: Bearer unit-test-key");
    assert(headers[2] == "anthropic-version: 2023-06-01");
    assert(headers[3] == "anthropic-workspace-id: wrkspc_unit_test");
    for (const auto & key : {"", "bad\r\nInjected: header", "bad key"}) {
      bool rejected = false;
      try { provider_headers("anthropic", key, ""); }
      catch (const std::exception &) { rejected = true; }
      assert(rejected);
    }
    bool rejected = false;
    try { provider_headers("anthropic", "unit-test-key", "workspace\nInjected"); }
    catch (const std::exception &) { rejected = true; }
    assert(rejected);
  }
  {
    const std::string url = "https://api.openai.com/v1/chat/completions";
    assert(provider_endpoint("openai", "", true) == url);
    assert(provider_endpoint("openai", url, true) == url);
    const auto headers = provider_headers("openai", "synthetic-test-credential", "");
    assert(headers.size() == 2);
    const auto colon = headers[1].find(": ");
    assert(headers[1].substr(0, colon) == "Authorization");
    assert(headers[1].substr(colon + 2) == std::string("Bearer") + " synthetic-test-credential");
    for (const auto & provider : {"anthropic", "openai"}) {
      const auto no_memory = nlohmann::json::parse(build_provider_request(
          provider, "test-model", "hello\n\"friend\"", {"private fact"}, 1, 20, 128, false));
      assert(no_memory.dump().find("private fact") == std::string::npos);
      const auto with_memory = nlohmann::json::parse(build_provider_request(
          provider, "test-model", "hello", {"private fact", "excluded"}, 1, 7, 128, true));
      assert(with_memory.dump().find("private") != std::string::npos);
      assert(with_memory.dump().find("private fact") == std::string::npos);
      assert(with_memory.dump().find("excluded") == std::string::npos);
      for (const int limit : {0, -1, 4097}) {
        bool rejected = false;
        try { build_provider_request(provider, "test-model", "hi", {}, 1, 20, limit, false); }
        catch (const std::exception &) { rejected = true; }
        assert(rejected);
      }
      bool rejected = false;
      try { provider_endpoint(provider, "", false); }
      catch (const std::exception &) { rejected = true; }
      assert(rejected);
      for (const auto & key : {"", "bad\r\nInjected: header", "bad key", "bad\tkey"}) {
        rejected = false;
        try { provider_headers(provider, key, ""); }
        catch (const std::exception &) { rejected = true; }
        assert(rejected);
      }
    }
    const auto body = nlohmann::json::parse(build_provider_request(
        "openai", "test-model", "hello\n\"friend\"", {}, 1, 20, 128, false));
    assert(body.at("max_completion_tokens") == 128);
    assert(body.at("store") == false);
    assert(body.at("stream") == false);
    assert(!body.contains("max_tokens") && !body.contains("system"));
    assert(body.at("messages").size() == 2);
    assert(body.at("messages")[0]["role"] == "developer");
    assert(body.at("messages")[1]["content"] == "hello\n\"friend\"");
    assert(parse_provider_reply("openai", R"({"choices":[{"message":{"content":" hi "}}]})") == "hi");
    for (const auto & bad : {"broken", R"({"error":{"message":"private"}})",
        R"({"choices":[]})", R"({"choices":[{"message":{"content":null,"tool_calls":[]}}]})",
        R"({"choices":[{"message":{"content":"   "}}]})"}) {
      bool rejected = false;
      try { parse_provider_reply("openai", bad); }
      catch (const std::exception &) { rejected = true; }
      assert(rejected);
    }
    for (const auto & bad_url : {"http://api.openai.com/v1/chat/completions",
        "https://api.openai.com.evil/v1/chat/completions", "https://api.openai.com/v1/chat/completions?key=fake",
        "https://api.anthropic.com/v1/messages"}) {
      bool rejected = false;
      try { provider_endpoint("openai", bad_url, true); }
      catch (const std::exception &) { rejected = true; }
      assert(rejected);
    }
    // Cloud secrets cannot accompany any local request.
    assert(provider_headers("local", "synthetic-test-credential", "test-workspace") ==
           std::vector<std::string>{"Content-Type: application/json"});
    for (const auto & bad_url : {"http://localhost:0/a", "http://localhost:65536/a",
        "http://localhost:80/a?x=1", "http://localhost:80\\evil/a", "http://localhost/a",
        "http://user@localhost:80/a", "http://127.0.0.1.evil:80/a", "https://localhost:80/a"}) {
      bool rejected = false;
      try { provider_endpoint("local", bad_url, false); }
      catch (const std::exception &) { rejected = true; }
      assert(rejected);
    }
  }
  std::cout << "conversation_core_test: PASS\n";
}
