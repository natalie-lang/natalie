# Experimental LLVM Backend

## Core Files

- `lib/natalie/compiler/backends/llvm_backend.rb`: compiler integration and linking.
- `lib/natalie/compiler/backends/llvm_backend/ir.rb`: shared LLVM emission state and function generation.
- `lib/natalie/compiler/backends/llvm_backend/native.rb`: LLVM C API bindings and object emission.
- `lib/natalie/compiler/instructions/`: instruction-specific `generate_llvm` methods.
- `tools/llvm_runtime.cpp` and `include/natalie/llvm_runtime.hpp`: precompiled runtime bridge.
- `test/llvm_backend_test.rb`: host Ruby integration tests.
- `examples/llvm_poc.rb`: supported-feature example.

Keep instruction-specific generation in the instruction classes. Format the bridge
explicitly with `clang-format` when editing it.

## Build and Run

After building Natalie, build the runtime bridge and run a script with LLVM:

```sh
bundle exec rake llvm_runtime
bin/natalie --backend=llvm examples/llvm_poc.rb
bundle exec rake test_llvm
```

LLVM 16 or newer must be available as a shared library. Set `NAT_LLVM_LIBRARY`
to its path if it cannot be found automatically (`nix develop` sets this).
Use `--emit-llvm` to inspect IR, or `--backend=llvm -c program script.rb` to
produce an executable. Script compilation emits LLVM object code and links
precompiled runtime objects; it does not generate or compile C++ for the script.

## Supported Features and Boundaries

The proof of concept supports literals, arrays, hashes, ranges, conditionals,
short-circuit expressions, while loops, local/global/instance variables,
constants, classes/modules, methods with positional/default/rest arguments,
blocks and lambdas, captured locals, `yield`, and statically resolved Ruby
`require`, `require_relative`, and `load`. Escaping closures share locals through
Natalie's existing GC-managed environments.

Unsupported instructions fail explicitly. Remaining gaps include rescue/ensure,
non-local block returns, `break`, loop `next`, keyword parameters, splat/forwarded calls,
inline C++ and native extensions, and complete parameter reflection. REPL,
shared-object output, and `--build-dir` are also unsupported.

The normal ruby/spec runner is **not yet supported**. For example,
`bin/natalie --backend=llvm spec/core/string/strip_spec.rb` stops at the runner's
inline C++ dependencies (including `tempfile`). Supporting that runner requires
a native-extension integration strategy and exception handling in the LLVM
backend. The LLVM tests exercise supported Ruby programs directly; they do not
replace the spec runner or silently use the C++ backend.
