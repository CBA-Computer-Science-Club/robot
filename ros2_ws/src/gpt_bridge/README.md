# gpt_bridge (ROS 2 Jazzy, Ubuntu 24.04)

A local, OpenAI-compatible **HTTP** chat-completions client. No cloud key and no shell commands. Install `libcurl4-openssl-dev` and build with `colcon build --packages-up-to gpt_bridge`. Run a local server exposing `/v1/chat/completions` (for example Ollama's local OpenAI-compatible API) and set the `model` parameter to a locally installed model. Only loopback HTTP endpoints with an explicit port are accepted; redirects and proxies are disabled.

```sh
ros2 run gpt_bridge gpt_bridge_node --ros-args \
  -p endpoint:=http://127.0.0.1:11434/v1/chat/completions \
  -p model:=llama3.2
```

`audio/heard` (`std_msgs/msg/String`) is **already transcribed text**, not raw audio. An external microphone/STT adapter must publish it. `robot/say` (`std_msgs/msg/String`) is text to speak; an external TTS/speaker adapter must subscribe. Neither voice adapter is supplied here. `people/identity` (`std_msgs/msg/String`) can carry `person_id` as a plain ID or JSON `{"person_id":"person_123"}`. Accepted IDs are 1–64 ASCII letters, digits, underscores or hyphens. A topic message is **not identity verification or consent** by itself.

Memory is off by default. To opt in for a verified, single-user operator-controlled session, set `memory_consent:=true`, `consented_person_id:=person_123`, and `identity_mode:=operator_bound`. The operator is responsible for binding that microphone/session to the named, consenting person. Alternatively, set `identity_mode:=trusted_topic` **only when** an external authenticated identity provider controls `people/identity`; messages must match the configured ID and be fresh (`identity_ttl_ms`, default 15000). Do not enable this mode with an unauthenticated public topic. `identity_mode:=disabled` disables memory despite consent; a topic message alone never enables memory. Memory never automatically saves conversations or model replies.

Only `Remember: <short fact>` (case-insensitive prefix; max 180 bytes) writes memory, after authorization. Facts are stored as a JSON array under person `consented_person_id` and key `facts` via `memory/get` and `memory/add` services. At most 20 facts of 180 bytes are retained, evicting oldest until the serialized value fits the memory service's 2048-byte limit; at most three facts/900 bytes are sent as model context. The service interfaces are `GetMemory {string person_id, string key, bool consent} -> {bool found, string value, string message}` and `AddMemory {string person_id, string key, string value, bool consent} -> {bool success, string message}`; the bridge explicitly sets `consent=true` only after local authorization. A memory read failure is reported and conversation continues without context; a failed write or failed read-back is reported, **never acknowledged as saved**. These key/value services have no compare-and-swap: use a single writer per identity. Do not place sensitive material in facts unless the person explicitly agrees.

Bounds: 2048-byte incoming utterance, 16 KiB JSON request, 64 KiB HTTP response, 2048-byte spoken response. `http_timeout_ms` defaults to 12000 (clamped 1000–60000); `memory_timeout_ms` defaults to 800 (clamped 100–5000). Failures do not log transcript text, endpoint contents or HTTP response bodies. A multi-threaded executor lets service response callbacks run while the heard callback waits. If memory service is unavailable, requests time out rather than blocking indefinitely.

Pure C++ tests (no ROS):

```sh
c++ -std=c++17 -Iinclude tests/conversation_core_test.cpp src/conversation_core.cpp -o conversation_core_test
./conversation_core_test
```

The bundled nlohmann JSON single header is MIT-licensed (see `include/nlohmann/LICENSE.MIT`).
