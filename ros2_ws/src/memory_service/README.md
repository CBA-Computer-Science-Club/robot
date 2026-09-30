# memory_service

ROS 2 Jazzy service for **explicitly opted-in, per-person factual memory**. Storage is a single JSON file selected by the `storage_path` node parameter (default `memory_service.json` in the process working directory). Configure an **absolute path** in a private directory for deployment. Requires `nlohmann-json3-dev` on Ubuntu 24.04.

## Exact service contract

Services are relative names under the node namespace:

```text
memory/add — memory_service/srv/AddMemory
string person_id
string key
string value
bool consent
---
bool success
string message

memory/get — memory_service/srv/GetMemory
string person_id
string key
bool consent
---
bool found
string value
string message

memory/list — memory_service/srv/ListMemories
string person_id
uint32 limit
bool consent
---
bool success
string[] keys
string[] values
string message

memory/forget — memory_service/srv/ForgetPerson
string person_id
---
bool success
string message
```

`person_id` identifies a single owner (not a name or biometric matching result). Add requires `consent=true` and a nonempty ID, key and value. Get and list require **current** `consent=true`; get performs exact key lookup only (no fuzzy matches or invented facts). List returns aligned key/value arrays sorted by key, at most `min(limit, 20)` records; zero limit returns no records. Forget needs no consent and is idempotent (`success=true` even when already empty); it removes that person's stored facts and leaves all other owners unchanged. Reject IDs/keys longer than 128 bytes and values longer than 2048 bytes; at most 128 facts per person. Add overwrites the same person's existing key.

**Trust boundary:** Consent is a caller assertion, not independently verified by this service. The invoking application must establish identity, capture a genuine opt-in before sending biometric or other personal data, re-check consent for every read, and restrict ROS graph access; ROS service calls do not authenticate `person_id`. Do not store face embeddings/images by default. Do not pass untrusted or hallucinated facts as `value`. Forget removes the live JSON entries but cannot guarantee forensic erasure of backups, filesystem snapshots, or previously created crash-recovery temp files. No encryption at rest is provided.

Storage uses copy-on-write to a uniquely named same-directory temporary file, flushes it, and atomically replaces the destination; on POSIX it creates mode `0600` files and syncs the containing directory. A failed write does not update the in-process store. Corrupt/unsupported files fail startup rather than silently clearing memory. Existing legacy flat-key JSON is **not migrated** automatically because it has no per-person ownership or consent provenance: archive it securely and obtain fresh consent before importing any facts. Run one writer process per storage path (cross-process locking is not provided).

## Verification

On Ubuntu with `nlohmann-json3-dev`, standalone tests do not require ROS:

```bash
cmake -S tests -B tests/.build/cmake
cmake --build tests/.build/cmake
ctest --test-dir tests/.build/cmake --output-on-failure
python3 -m unittest discover -s tests -p 'test_interfaces.py' -v
```

ROS build on the target host (not executable on the Windows development machine):

```bash
cd ros2_ws
colcon build --packages-select memory_service
source install/setup.bash
ros2 run memory_service memory_service_node --ros-args -p storage_path:=/path/to/private/memory.json
```

Local Windows standalone verification: MSVC 19.50 / Visual Studio 18 generated with CMake; `nlohmann/json.hpp` 3.11.3 fetched into ignored `tests/.build/include`; `ctest` ran `1/1` C++ test executable passing **6 behavior cases**, and Python interface-contract tests ran `2/2` passing. ROS/colcon compilation was not available here and remains unverified on Ubuntu/Jazzy.
