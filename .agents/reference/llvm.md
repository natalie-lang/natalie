# Experimental LLVM Backend

## Core Files

- `lib/natalie/compiler/backends/llvm_backend.rb`: compiler integration and linking.
- `lib/natalie/compiler/backends/llvm_backend/ir.rb`: shared LLVM emission state and function generation.
- `lib/natalie/compiler/backends/llvm_backend/native.rb`: LLVM C API bindings and object emission.
- `lib/natalie/compiler/backends/llvm_backend/native_extensions.rb`: C++ extension objects and ABI wrappers.
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
precompiled runtime objects. Ruby bodies stay in LLVM. Programs using native
extensions or inline C++ macros additionally compile a separate C++ object with
the native sources and registration/call wrappers. Pure Ruby scripts still use
the C++ driver only for linking.

## Supported Features and Boundaries

The proof of concept supports literals, arrays, hashes, ranges, conditionals,
short-circuit expressions, case matching, string interpolation, regexp literals
and interpolation, capture access, while loops, local/global/instance/class variables,
constants, `defined?` probes, classes/modules and singleton classes, method/global
aliases, `undef`, `super`,
methods with positional/default/rest and keyword arguments,
blocks and lambdas, captured locals, anonymous argument/block forwarding, splat
calls and yields, rescue/ensure/retry, loop and block jumps, non-local and
top-level returns, and statically resolved Ruby
`require`, `require_relative`, and `load`. Escaping closures share locals through
Natalie's existing GC-managed environments.

## Native Extensions

`require 'extension.cpp'` uses Natalie's existing extension convention:
`init_extension(Env *, Value)` plus inline macros such as `__bind_method__`.
The sidecar object reuses `CppBackend::Transform` for these macros and
`CppBackend::OutFile` for compilation flags and diagnostics. Initialization runs
before the LLVM entrypoint's Ruby instructions, matching the C++ backend.

LLVM calls generated `extern "C"` wrappers using tagged machine words and
pointers. C++ `Value`, `Args`, and method-function calling conventions stay
inside C++. Native methods can call back into LLVM blocks through the existing
runtime bridge. Object-file output combines the LLVM and native objects into a
single relocatable object using the C++ driver; executable output links them
separately. `--emit-llvm` does not invoke the C++ compiler, but its IR references
the sidecar when one is needed.

The existing generator handles method/static-method bindings, allocation hooks,
constants, native method definitions, function declarations/calls, and compile/link
flags. Top-level inline declarations are supported; scoped `__inline__` code is
rejected because it can refer to C++ local variables that do not exist in LLVM.

Tests include a native extension fixture, C++-to-LLVM block callbacks, linking a
combined object, and using the actual `tempfile.cpp` implementation directly.
The full Ruby `tempfile` library is also exercised through `Tempfile.create`.

Exception bodies and handlers compile to LLVM callbacks that share their caller's
environment. The C++ bridge catches Ruby exceptions, restores `$!`, runs ensure
handlers, and reuses Natalie's local-jump exceptions for non-local control flow.

Loop branches remain in LLVM; scoped body/condition callbacks use the bridge to
translate local jumps into branch statuses after intervening ensure handlers run.

Unsupported instructions fail explicitly. Remaining gaps include once-only
regexp interpolation (`/o`), scoped inline C++, and complete parameter reflection. REPL, shared-object output, and `--build-dir` are also unsupported.

The normal ruby/spec runner now works for the unmodified
`spec/core/nil/nil_spec.rb`:

```sh
bin/natalie --backend=llvm spec/core/nil/nil_spec.rb
# 1 spec(s) passed.
bin/natalie --backend=llvm spec/language/keyword_arguments_spec.rb
# 22 spec(s) passed; 14 existing skips (same result as the C++ backend).
```

This compiles the shared helpers and their Ruby/native dependencies through the
LLVM and native-extension paths. The LLVM integration tests also verify that an
assertion failure is reported as a failed spec with exit status 1. This proves a
real runner path, not general ruby/spec conformance; other specs can still use
unsupported instructions or encounter incomplete behavior.
