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
and interpolation, while loops, local/global/instance variables,
constants, classes/modules, methods with positional/default/rest arguments,
blocks and lambdas, captured locals, anonymous argument/block forwarding, splat
calls and yields, and statically resolved Ruby
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
The full Ruby `tempfile` library still uses unsupported Ruby constructs.

Unsupported instructions fail explicitly. Remaining gaps include rescue/ensure,
non-local block returns, `break`, loop `next`, keyword parameters, once-only
regexp interpolation (`/o`),
scoped inline C++, and complete parameter reflection. REPL,
shared-object output, and `--build-dir` are also unsupported.

The normal ruby/spec runner is **not yet supported**. For example,
`bin/natalie --backend=llvm spec/core/string/strip_spec.rb` stops at the runner's
block `break` handling, after passing native-extension, splat, and regexp
compilation. Supporting that runner still requires more Ruby
instructions, including forwarding and exception handling. The LLVM tests
exercise supported Ruby programs directly; they do not replace the spec runner
or silently use the C++ backend.
