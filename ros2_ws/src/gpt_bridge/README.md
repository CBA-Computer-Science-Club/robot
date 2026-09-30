# gpt_bridge (ROS 2 Jazzy, Ubuntu 24.04)

A C++ text-conversation client supporting three selectable providers: **local** OpenAI-compatible inference (default), **Anthropic** native Messages API, and **OpenAI** Chat Completions API. Install `libcurl4-openssl-dev` and build with `colcon build --packages-up-to gpt_bridge`. No Python SDK or shell commands are used by the node.

## Provider configuration

| `provider` | Default model when `model` is empty | Credential environment variable |
|---|---|---|
| `local` | `llama3.2` | None; cloud credentials are never attached |
| `anthropic` | `claude-haiku-4-5` | `ANTHROPIC_API_KEY` |
| `openai` | `gpt-4.1-mini` | `OPENAI_API_KEY` |

`endpoint` and `model` default to empty strings in the node and launch file so provider-specific defaults are selected correctly. Override `model` with a text-capable model your account can access; for OpenAI it must support Chat Completions and developer messages. These defaults are not a promise of the newest model or continuing account access. Local inference still requires a separately installed model/server.

Cloud providers require `allow_cloud_api:=true` **and** a nonempty valid key inherited by the node process. Missing keys, malformed headers, unknown providers, and unapproved endpoints fail startup before subscribing to transcripts. There is no automatic local-to-cloud fallback. `ANTHROPIC_WORKSPACE_ID` is optional unless your Anthropic key requires a workspace selector. OpenAI never receives this Anthropic-specific header. API keys are not ROS parameters or launch arguments; do not paste them into ROS commands, logs, source files, or chat. Environment files are not automatically loaded.

Local URLs must be loopback HTTP with an explicit valid port; the default is `http://127.0.0.1:11434/v1/chat/completions`. Cloud URLs are restricted to the exact direct endpoints `https://api.anthropic.com/v1/messages` and `https://api.openai.com/v1/chat/completions` for the selected provider. Custom proxies, Azure/Bedrock/Vertex gateways and alternate URLs are not supported. Cloud transport requires verified HTTPS certificates and hostnames. Redirects and environment proxy use are disabled for every provider. Requests are non-streaming, text-only, with no tool execution.

### Local

```sh
ros2 run gpt_bridge gpt_bridge_node --ros-args \
  -p endpoint:=http://127.0.0.1:11434/v1/chat/completions \
  -p model:=llama3.2
```

### Anthropic / Claude (on the Ubuntu robot)

Set the key in the same shell that launches ROS, using a hidden prompt rather than putting a literal key into shell history:

```bash
read -rsp 'Anthropic API key: ' ANTHROPIC_API_KEY; printf '\n'
export ANTHROPIC_API_KEY
# Set ANTHROPIC_WORKSPACE_ID through your environment if your key requires it.
ros2 launch robot_bringup robot.launch.py provider:=anthropic allow_cloud_api:=true
```

To run just the conversation node instead: `ros2 run gpt_bridge gpt_bridge_node --ros-args -p provider:=anthropic -p allow_cloud_api:=true`.

### OpenAI (on the Ubuntu robot)

```bash
read -rsp 'OpenAI API key: ' OPENAI_API_KEY; printf '\n'
export OPENAI_API_KEY
ros2 launch robot_bringup robot.launch.py provider:=openai allow_cloud_api:=true
```

To run just the conversation node instead: `ros2 run gpt_bridge gpt_bridge_node --ros-args -p provider:=openai -p allow_cloud_api:=true`.

After editing this repository, rebuild the workspace and source `install/setup.bash` before using these commands. Cloud requests require internet access, account/model access and paid API capacity; provider billing and data-handling policies apply. `max_tokens` (default 256, range 1-4096) limits cloud output: Anthropic uses `max_tokens`, OpenAI uses `max_completion_tokens`. OpenAI requests set `store: false`; this does **not** assert zero provider retention or override provider policies. There are no automatic retries, provider failovers, streaming, audio, image or tool requests.

## Topics and consent

`audio/heard` (`std_msgs/msg/String`) is **already transcribed text**, not raw audio. An external microphone/STT adapter must publish it. `robot/say` (`std_msgs/msg/String`) is text to speak; an external TTS/speaker adapter must subscribe. Neither voice adapter is supplied here. `people/identity` (`std_msgs/msg/String`) can carry `person_id` as a plain ID or JSON `{"person_id":"person_123"}`. Accepted IDs are 1–64 ASCII letters, digits, underscores or hyphens. A topic message is **not identity verification or consent** by itself.

**Cloud disclosure:** `allow_cloud_api` permits sending the current utterance and the assistant's system/developer prompt to the selected provider. Saved facts remain local unless `allow_cloud_memory:=true` is separately enabled **and** existing per-person memory authorization succeeds. That gate is enforced both before retrieval and in request construction. Biometric data and person IDs are not added to model requests. Explicit `Remember:` commands are processed locally and never sent to a model. Obtain informed consent before transmitting anyone's conversations.

Memory is off by default. To opt in for a verified, single-user operator-controlled session, set `memory_consent:=true`, `consented_person_id:=person_123`, and `identity_mode:=operator_bound`. The operator is responsible for binding that microphone/session to the named, consenting person. Alternatively, set `identity_mode:=trusted_topic` **only when** an external authenticated identity provider controls `people/identity`; messages must match the configured ID and be fresh (`identity_ttl_ms`, default 15000). Do not enable this mode with an unauthenticated public topic. `identity_mode:=disabled` disables memory despite consent; a topic message alone never enables memory. Memory never automatically saves conversations or model replies.

Only `Remember: <short fact>` (case-insensitive prefix; max 180 bytes) writes memory, after authorization. Facts are stored as a JSON array under person `consented_person_id` and key `facts` via `memory/get` and `memory/add` services. At most 20 facts of 180 bytes are retained, evicting oldest until the serialized value fits the memory service's 2048-byte limit; at most three facts/900 bytes are sent as model context. The service interfaces are `GetMemory {string person_id, string key, bool consent} -> {bool found, string value, string message}` and `AddMemory {string person_id, string key, string value, bool consent} -> {bool success, string message}`; the bridge explicitly sets `consent=true` only after local authorization. A memory read failure is reported and conversation continues without context; a failed write or failed read-back is reported, **never acknowledged as saved**. These key/value services have no compare-and-swap: use a single writer per identity. Do not place sensitive material in facts unless the person explicitly agrees.

Bounds: 2048-byte incoming utterance, 16 KiB JSON request, 64 KiB HTTP response, 2048-byte spoken response. `http_timeout_ms` defaults to 12000 (clamped 1000–60000); `memory_timeout_ms` defaults to 800 (clamped 100–5000). Failures do not log transcript text, endpoint contents or HTTP response bodies. A multi-threaded executor lets service response callbacks run while the heard callback waits. If memory service is unavailable, requests time out rather than blocking indefinitely.

## Offline verification

Pure C++ tests (no ROS; synthetic fixtures only, no API credentials or billed calls):

```sh
cmake -S tests -B /tmp/chat-tests
cmake --build /tmp/chat-tests --config Debug
ctest --test-dir /tmp/chat-tests -C Debug --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py' -v
```

The C++ suite exercises JSON schemas, authorization-header construction, endpoint allowlists, invalid credentials, response parsing, memory bounds and local-provider compatibility. Python source contracts additionally check node/launch wiring; they are **not** a compiled ROS integration or TLS-network test. Build and test on ROS 2 Jazzy/Ubuntu before deployment, then test the selected API with an authorized key and non-sensitive input. Windows development tests cannot establish Pi/ROS or live authenticated behavior.

Protocol references: [Anthropic API authentication](https://platform.claude.com/docs/en/api/overview), [Messages API](https://platform.claude.com/docs/en/api/messages/create), [Claude model IDs](https://platform.claude.com/docs/en/models/overview), [OpenAI Chat Completions](https://developers.openai.com/api/reference/python/resources/chat/subresources/completions/methods/create/), and [GPT-4.1 mini](https://developers.openai.com/api/docs/models/gpt-4.1-mini). The implementation uses native REST with libcurl, not those Python SDKs.

The bundled nlohmann JSON single header is MIT-licensed (see `include/nlohmann/LICENSE.MIT`).
