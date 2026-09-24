# LlmComponent

A C++ component for integrating Large Language Model (LLM) capabilities via [Ollama](https://ollama.com/). It provides a worker architecture for processing LLM queries asynchronously with bounded session memory, plus a config-driven Ollama provider with optional image (vision) support.

## Highlights

- **Dependency-injected providers** behind a minimal `iLlmProvider` interface.
- **Async worker** (`cBaseWorker_V2` sibling component) with a bounded, back-pressure-aware prompt queue.
- **Config-driven system prompt** via `LlmConfig` (single source of truth — no domain-specific strings baked into the provider).
- **Optional vision**: image files are Base64-encoded and attached to the Ollama vision message array.
- **Pure, testable core**: chat-request building, reply parsing, and Base64 encoding are free of network/I/O and covered by unit tests.
- **Self-contained Base64** — the previous undeclared drogon dependency (and its missing link declaration) is gone.

## Dependencies

- CMake 3.16+
- C++17 compiler
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp) (Homebrew) — discovered via pkg-config first, with the upstream CMake config as fallback
- [cURL](https://curl.se/) (Homebrew)
- [Ollama](https://ollama.com/) running locally
- ThreadComponent (sibling module, for `cBaseWorker_V2` and `cHTTPClient`)

```bash
brew install cmake pkg-config jsoncpp curl
```

## Building

From this directory:

```bash
cmake -S . -B build
cmake --build build
```

The executable `LocalBrainIntegrationTest` is produced in `build/`. Run the unit tests with:

```bash
ctest --test-dir build
```

## Running

Ensure Ollama is running (`ollama serve` or the desktop app) and a model is pulled:

```bash
ollama pull llama3.2
./build/LocalBrainIntegrationTest
```

The endpoint and model resolve from the command line first, then environment variables, then defaults:

```bash
./build/LocalBrainIntegrationTest http://localhost:11434 llama3.2
OLLAMA_ENDPOINT=http://localhost:11434 OLLAMA_MODEL=llama3.2 ./build/LocalBrainIntegrationTest
```

For a vision model (e.g. `qwen2.5vl`), pass the model name and use `request_response(prompt, imagePath)`.

## Project Layout

```text
LlmComponent/
|-- CMakeLists.txt            # llm_core static lib + LocalBrainIntegrationTest + llm_tests
|-- README.md
|-- .gitignore
|-- llm_config.hpp            # LlmConfig struct
|-- base64_encoder.hpp        # pure Base64 helper (replaces drogon)
|-- llm_payload.hpp/.cpp      # pure request builder + reply parser + directive protocol
|-- iLlmProvider.hpp          # provider interface
|-- cOllamaProvider.hpp/.cpp  # thin config-driven Ollama provider (HTTP via cHTTPClient)
|-- cSessionMemory.hpp        # thread-safe, bounded conversational history
|-- cLlmBrainWorker.hpp/.cpp  # async worker composition (bounded queue, callback)
|-- main_brain_test.cpp       # demo app, endpoint/model resolution
`-- tests/
    `-- test_llm.cpp          # unit tests (no network)
```

The component also expects the sibling `ThreadComponent` project at `../ThreadComponent`.

## Public Interface

### LlmConfig

```cpp
struct LlmConfig
{
    std::string endpoint = "http://localhost:11434";
    std::string model = "llama3.2";
    std::string systemPrompt;   // single source of truth for the persona
    float temperature = 0.0f;   // 0 = leave server defaults
    int numPredict = 0;
    std::int32_t connectTimeoutMs = 10000;
    std::int32_t requestTimeoutMs = 180000;
    std::size_t maxHistoryTurns = 10;
    std::size_t maxQueueDepth = 32;
    std::size_t maxPromptChars = 65536;
};
```

### Worker

```cpp
using LlmResponseCallback = std::function<void(const std::string &replyText)>;

cLlmBrainWorker(std::shared_ptr<iLlmProvider> provider, const LlmConfig &cfg,
                LlmResponseCallback callback = nullptr);

void request_response(const std::string &userPrompt, const std::string &imagePath = "");
void set_callback(LlmResponseCallback callback);
```

`request_response` is non-blocking; replies arrive on the worker thread through
the callback. The worker keeps the last `maxHistoryTurns` turns in session memory
automatically, and drops the oldest queued request (with a throttled warning) when
the queue exceeds `maxQueueDepth`.

### System directive protocol

An utterance beginning with `System Prompt Directive:` is appended to the system
prompt rather than sent as a user message (used for lesson-state style
instructions). The marker is `kSystemDirectiveMarker` and the behavior is
implemented in the pure `augment_system_prompt()`.

## Architecture

- **cLlmBrainWorker** — async worker: bounded prompt queue, session memory, callback dispatch.
- **cOllamaProvider** — `iLlmProvider` implementation for the Ollama `/api/chat` endpoint; config-driven system prompt; vision via Base64 image payloads.
- **cSessionMemory** — mutex-guarded, turn-capped conversational history (no system message; that lives in `LlmConfig`).
- **iLlmProvider** — the provider interface (`query`, `name`).

## Troubleshooting

### Init fails / connection refused

Make sure Ollama is running and the endpoint is reachable:

```bash
curl http://localhost:11434/api/version
ollama list
```

### Model not found

Pull the model before running:

```bash
ollama pull llama3.2
```

### Failure handling

Transport, parse, and payload-shape failures are surfaced as exceptions from
`cOllamaProvider::query`; the worker logs them and continues. The demo exits when
initialization fails.