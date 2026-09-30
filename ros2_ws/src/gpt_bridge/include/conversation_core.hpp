#pragma once

#include <nlohmann/json.hpp>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace gpt_bridge {

// These helpers are ROS-independent, so they can be tested with a plain C++ compiler.
std::string build_request(const std::string & model, const std::string & utterance,
                          const std::vector<std::string> & memories,
                          std::size_t max_memories, std::size_t max_context_chars);
std::string parse_reply(const std::string & response);
std::string provider_endpoint(const std::string & provider, const std::string & configured,
                              bool allow_cloud_api);
std::vector<std::string> provider_headers(const std::string & provider, const std::string & api_key,
                                         const std::string & workspace_id);
std::string parse_provider_reply(const std::string & provider, const std::string & response);
std::string build_provider_request(const std::string & provider, const std::string & model,
    const std::string & utterance, const std::vector<std::string> & memories,
    std::size_t max_memories, std::size_t max_context_chars, int max_tokens,
    bool allow_cloud_memory);
std::optional<std::string> parse_remember(const std::string & utterance,
                                          std::size_t max_fact_chars);
// Returns empty for an invalid person ID. An identity message is a hint, not proof.
std::string parse_identity(const std::string & message);
// Stored value is a JSON array of short, explicitly saved facts.
std::vector<std::string> read_facts(const std::string & value, std::size_t max_count,
                                    std::size_t max_fact_chars);
std::string append_fact(const std::string & value, const std::string & fact,
                        std::size_t max_count, std::size_t max_fact_chars);

}  // namespace gpt_bridge
