# LlmComponent

A C++ component for integrating Large Language Model (LLM) capabilities via Ollama. This module provides a brain worker architecture for processing LLM queries with session memory management.

## Dependencies

- CMake 3.16+
- C++17 compiler
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp) (via Homebrew)
- [cURL](https://curl.se/) (via Homebrew)
- [Ollama](https://ollama.com/) running locally
- ThreadComponent (sibling module)

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

The executable `LocalBrainIntegrationTest` will be created in the build directory. Ensure Ollama is running locally before execution.

```bash
./LocalBrainIntegrationTest
```

## Architecture

- **cLlmBrainWorker** - Main worker class for LLM brain operations
- **cOllamaProvider** - Provider implementation for Ollama API
- **cSessionMemory** - Session memory management
- **iLlmProvider** - Interface for LLM providers