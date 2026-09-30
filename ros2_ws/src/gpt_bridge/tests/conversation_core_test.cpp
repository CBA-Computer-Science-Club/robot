#include "conversation_core.hpp"
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
  std::cout << "conversation_core_test: PASS\n";
}
