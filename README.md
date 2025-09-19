# Robot — Voice-Controlled Robot Assistant

Small in-process framework that connects a speech front-end to an OpenAI-powered
assistant and a simple robot control event bus. The project is intentionally
minimal and intended for local development on Raspberry Pi / desktop systems.

## Features

- Speech-to-text -> text events handled by `modules/gpt_module.py`.
- Uses OpenAI chat completions with `TOOLS` integration to allow the assistant
  to call structured functions that map to robot actions.
- Simple `EventBus` for publishing and subscribing to events across modules.
- Pluggable modules for listening, speaking and robot actions.

## Quick Start

Prerequisites:

- Python 3.10+ (recommended)
- `OPENAI_API_KEY` environment variable set to a valid key

Install dependencies (if any are used in your environment):

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt  
```

Set the OpenAI API key and run the main script:

```bash
export OPENAI_API_KEY="sk-..."
python main.py
```

Note: This project expects a small set of resource files to be present:
- `resources/system_prompt.txt` — system prompt used to steer the assistant

## Configuration

- `resources/system_prompt.txt`: Controls assistant behaviour (system message).
- `modules/gpt_module.py`: Selects the model via the `_model` variable.

## Project Layout

- `main.py` — application entrypoint (wire up bus and start modules).
- `core/` — core utilities:
  - `event_bus.py` — a queue-backed EventBus used for cross-module events.
  - `loader.py` — (loader utilities)
- `modules/` — pluggable components handling speech, GPT, and robot actions.
  - `gpt_module.py` — integrates OpenAI chat completions and maps tool calls
    to robot events.
  - `listen_module.py` — speech-to-text integration (broadcasts `audio.heard`).
  - `speech_module.py` — text-to-speech (listens for `audio.speak`).
  - `startup_module.py` — startup routines.
  - `time_module.py` — periodic time events.
- `events/` — higher-level event definitions and small handlers.
- `resources/` — static assets such as `system_prompt.txt`.

## Event Bus API

The `EventBus` in `core/event_bus.py` implements a simple pub/sub model:

- `broadcast(event_type, **kwargs)` — publish an event.
- `subscribe(event_type, callback)` — register a persistent listener.
- `once(event_type, callback)` — register a one-time listener.
- `unsubscribe(event_type, callback)` — remove a listener.
- `dispatch_loop()` — the blocking consumer loop that should be run on a
  dedicated thread to process events.

Listeners receive event payloads as keyword arguments.

## Extending the Robot

- Add new tools in `tools.py` to make them available to the assistant.
- Register additional modules under `modules/` and connect them to the bus.

## Troubleshooting

- If the assistant doesn't respond, ensure `OPENAI_API_KEY` is set and the
  model name in `gpt_module.py` is supported by your account.
- For audio problems, verify the speech modules are running and broadcasting
  `audio.heard` and `audio.speak` events.

## License

Add the license here
