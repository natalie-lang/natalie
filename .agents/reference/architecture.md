# Project Structure and Architecture

Natalie is an ahead-of-time Ruby implementation with a C++ runtime.

- `bin/natalie`: host Ruby compiler CLI; `bin/nat`: self-hosted compiler after bootstrapping.
- `lib/natalie/compiler/`: compiler passes, instructions, and backends. Instructions implement `generate` for C++ and `generate_llvm` for the experimental LLVM backend.
- `src/` and `include/natalie/`: runtime implementation, headers, and generated-code templates.
- `lib/`: Ruby standard-library implementations; `ext/`: bundled dependencies.
- `spec/`: imported ruby/spec conformance tests; `test/`: Natalie-specific tests and host Ruby integration tests.
- `examples/`: sample programs and assets; `tools/`: development utilities and LLVM runtime bridge; `build/`: generated artifacts.
