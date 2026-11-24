# Project Structure and Architecture

Natalie is an ahead-of-time Ruby implementation with a C++ runtime.

- `bin/natalie`: host Ruby compiler CLI; `bin/nat`: self-hosted compiler after bootstrapping.
- `lib/natalie/compiler/`: compiler passes, instructions, and the LLVM backend. Instructions implement `generate_llvm`; explicit native macros retain their C++ renderer.
- `lib/natalie/compiler/native_build.rb` and `native_build/`: platform settings, native source rendering, linking, and package flags. Keep these utilities independent of Ruby code generation.
- `src/` and `include/natalie/`: runtime implementation, headers, and generated-code templates.
- `lib/`: Ruby standard-library implementations; `ext/`: bundled dependencies.
- `spec/`: imported ruby/spec conformance tests; `test/`: Natalie-specific tests and host Ruby integration tests.
- `examples/`: sample programs and assets; `tools/`: development utilities and LLVM runtime bridge; `build/`: generated artifacts.
