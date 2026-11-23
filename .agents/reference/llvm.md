# Experimental LLVM Backend

## Core Files

- `lib/natalie/compiler/backends/llvm_backend.rb`: compiler integration and linking.
- `lib/natalie/compiler/backends/llvm_backend/ir.rb`: shared LLVM emission state and function generation.
- `lib/natalie/compiler/backends/llvm_backend/native.rb`: LLVM C API bindings and object emission.
- `lib/natalie/compiler/backends/llvm_backend/native_ffi.rb`: LLVM bindings for the self-hosted compiler.
- `lib/natalie/compiler/backends/llvm_backend/native_extensions.rb`: C++ extension objects and ABI wrappers.
- `lib/natalie/compiler/backends/llvm_backend/units.rb` and `build_cache.rb`: separate Ruby compilation units and cached artifacts.
- `lib/natalie/compiler/instructions/`: instruction-specific `generate_llvm` methods.
- `tools/llvm_runtime.cpp` and `include/natalie/llvm_runtime.hpp`: precompiled runtime bridge.
- `src/constant.cpp` and `include/natalie/constant.hpp`: deferred autoload callbacks and their GC roots.
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
the native sources and registration/call wrappers. Pure Ruby executables still use
the C++ driver only for linking.

The self-hosted compiler also supports LLVM:

```sh
bundle exec rake bootstrap llvm_runtime
bin/nat --backend=llvm examples/llvm_poc.rb
bundle exec rake test_llvm_self_hosted
```

MRI uses Fiddle to call the LLVM C API; `bin/nat` uses Natalie's existing
libffi bindings. Both use the same IR generator, object emitter, native
extension support, and build cache. LLVM is loaded at runtime, so bootstrapping
the compiler and using its default C++ backend do not require LLVM to be installed.
The self-hosted test task runs the backend and build-cache integration suites
through `bin/nat` (the direct malformed-IR unit tests still run under MRI).
A compiled fixture also checks parser, verifier, and object-emission errors and
recovery through Natalie's FFI adapter.

Use `-d code` to print generated LLVM IR and any native sidecar sources.
`--keep-source` retains intermediate source files and prints their paths;
cached builds retain IR in their build directory. `-d S` emits and prints assembly
for each Ruby compilation unit and any native sidecar.

## REPL

Build the compiler library and LLVM bridge, then start the REPL without a script:

```sh
bundle exec rake libnat llvm_runtime
bin/natalie --backend=llvm
NAT_REPL_TEST_BACKEND=llvm bundle exec ruby test/ruby/repl_test.rb
```

The CLI passes its backend selection to the running REPL. Each input compiles to
an LLVM shared library with the same `EVAL` entrypoint used by the C++ REPL.
Inputs share a growing local-variable vector, preserving earlier values and
closure captures. Native extension initializers run before evaluating the input.
The LLVM test task includes the shared REPL regression tests; the ordinary Ruby
test suite runs those tests with the default C++ backend.

## Cached Builds

Use `--build-dir` for separate compilation and reusable object files:

```sh
bin/natalie --backend=llvm --build-dir=build/my-program program.rb
```

Each required Ruby file has its own LLVM module and object. File entrypoints and
file-return tokens cross object boundaries; methods, closures, literal caches,
and other jump tokens stay private to their module. Native extension wrappers
use stable per-file names, and their C++ code is cached in a separate object.

Artifacts live under `BUILD_DIR/llvm/`, grouped by the main source path and
output mode. The cache retains `.ll`, `.o`, native `.cpp`/`.d`, and fingerprint
manifests. Ruby objects are rebuilt when their IR or LLVM toolchain identity
changes. Native objects also track compiler flags, compiler identity, and
transitive project header dependencies using content fingerprints. Executable,
combined object, and shared-library output all support the cache.

Each build regenerates IR to validate cache keys and relinks the output.
Atomic replacements and a persistent per-program lock protect concurrent
builders. `--build-quietly` suppresses build progress; `NAT_VERBOSE=1` reports
reused objects when progress is enabled. C++ and LLVM can share a build directory
because LLVM artifacts have their own namespace.

## Supported Features and Boundaries

The backend supports literals (including rational and complex numbers), arrays,
hashes, ranges, conditionals,
short-circuit expressions, case matching, string interpolation, regexp literals
and interpolation (including `/o`), capture access, flip-flops, encoded strings,
frozen literal identity, while loops, local/global/instance/class variables,
constants, `defined?` probes, classes/modules and singleton classes, method/global
aliases, `undef`, `super`,
methods with positional/default/rest and keyword arguments, parameter metadata,
blocks and lambdas, captured locals, anonymous argument/block forwarding, splat
calls and yields, rescue/ensure/retry, loop and block jumps, non-local and
top-level returns, and statically resolved Ruby
`require`, `require_relative`, `load`, and autoload registration. Escaping closures share locals through
Natalie's existing GC-managed environments.

## Native Extensions

`require 'extension.cpp'` uses Natalie's existing extension convention:
`init_extension(Env *, Value)` plus inline macros such as `__bind_method__`.
The sidecar object reuses `CppBackend::Transform` for these macros and
`NativeBuild::SourceFile` for compilation flags and diagnostics. Initialization runs
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
flags. Top-level inline declarations are supported. Scoped `__inline__` code is
retired on both backends. Use explicit native helper calls or native method
definitions instead; see [Native code](native-code.md).

Tests include a native extension fixture, C++-to-LLVM block callbacks, linking a
combined object, and using the actual `tempfile.cpp` implementation directly.
The full Ruby `tempfile` library is also exercised through `Tempfile.create`.

## Shared Libraries

Compile a library using the same output mode as the C++ backend:

```sh
bin/natalie --backend=llvm --compilation-type=shared-object \
  -c build/widgets.so widgets.rb
```

The library exports the existing C++ `Value init_widgets(Env *, Value)` entrypoint
for this output path. It runs inside the calling process's Natalie runtime;
it does not create a runtime or contain an executable `main`. A small C++ shim
preserves the runtime's `Value` calling convention while Ruby bodies remain in
LLVM. The link includes the LLVM bridge and uses shared-library flags
automatically. Native extensions and their initialization are also supported.

Initialization preserves the caller's `$0`, returns `nil` normally, and honors
explicit Ruby returns and exceptions, including `ensure`. Generated LLVM
functions and native wrappers are local or hidden so multiple libraries can
be loaded into the same process. Non-local jumps use private address tokens
so independently compiled libraries cannot catch each other's returns or breaks.
Keep libraries loaded while their Ruby
methods, closures, or literal caches remain reachable. As with C++ output,
add an explicit `extern "C"` wrapper when exposing initialization through FFI
rather than linking to the C++ entrypoint directly.

Integration tests load two libraries into both C++ and LLVM hosts and exercise
native extension callbacks, GC, return values, exception propagation, and `$0`.

## Runtime Semantics

Exception bodies and handlers compile to LLVM callbacks that share their caller's
environment. The C++ bridge catches Ruby exceptions, restores `$!`, runs ensure
handlers, and reuses Natalie's local-jump exceptions for non-local control flow.

Loop branches remain in LLVM; scoped body/condition callbacks use the bridge to
translate local jumps into branch statuses after intervening ensure handlers run.

Literal caches live in the shared runtime, with GC roots and a recursive guard.
Both backends skip `/o` interpolation after successful initialization, retry it
after an exception or local jump, and serialize concurrent initialization.

Autoload uses the same compile-time restrictions as the C++ backend: bare
`autoload` calls with statically resolvable constant names and paths. The
constant holds a GC-traced loader block until lookup triggers the compiled
file. Literal lookup and `Module#const_get` trigger loading. If executing the
file raises, the registration and failed require state are reset so the file
can be retried. Loaded-file returns count as successful requires; explicit
`load` does not modify require's feature state.
Dynamic runtime `Module#autoload` calls remain outside this static macro path.

`--profile-app` and `--print-objects` use the shared native startup generator
with the same runtime instrumentation as the C++ backend. A configured startup
object is compiled only when these options are requested; ordinary executables
reuse the prebuilt startup object. Cached builds keep the configured startup in
`entrypoint.o`, independently of native extensions in `native.o`.

The normal ruby/spec runner now works for the unmodified
`spec/core/nil/nil_spec.rb`:

```sh
bin/natalie --backend=llvm spec/core/nil/nil_spec.rb
# 1 spec(s) passed.
bin/natalie --backend=llvm spec/language/keyword_arguments_spec.rb
# 22 spec(s) passed; 8 existing skips (same result as the C++ backend).
```

This compiles the shared helpers and their Ruby/native dependencies through the
LLVM and native-extension paths. The LLVM integration tests also verify that an
assertion failure is reported as a failed spec with exit status 1. This proves a
real runner path, not general ruby/spec conformance; other specs can still use
unsupported instructions or encounter incomplete behavior.

## Comparing Specs With C++

Run a selection through both backends, with bounded parallelism and per-file
compile/run deadlines:

```sh
bundle exec ruby tools/llvm_spec_runner.rb --jobs=4 --timeout=120 \
  --build-dir=build/spec-cache \
  --report=build/llvm-spec-results.json 'spec/language/*_spec.rb'
```

The runner sets `NAT_BINARY` to a backend-specific executable wrapper, so
`ruby_exe` child programs use the same backend as their parent spec. It records
exit status, signals, timeouts, full output, and passed/failed/errored/skipped
counts for each backend in JSON. A matching pass requires successful, nonempty
runs with identical counts. Empty runs and failures are listed separately and
never added to the matching passed-example total. Use `--backend=llvm` for a
single-backend run while diagnosing a failure.
The optional `--build-dir` gives each backend a separate cache and applies to
both parent specs and their child compiler invocations.

For the broader language, method/Proc, and numeric selection used during backend
development:

```sh
bundle exec ruby tools/llvm_spec_runner.rb --jobs=6 --timeout=180 \
  --report=build/llvm-spec-final.json 'spec/language/*_spec.rb' \
  'spec/core/{method,proc,integer,rational,complex,nil,true,false}/*_spec.rb'
```

This selection includes existing `NATFIXME` skips. Matching results demonstrate
coverage of the active examples, rather than full Ruby conformance. Files with
no active examples remain separate from successful spec files in the report.

Validation at `c8f10470d` (2026-11-23): all 65 language files matched, with 2,323
passing examples. Another 207 core files matched, with 1,301 passing examples.
Two core files had no active examples. The full selection retained 598 skips.
The LLVM backend and runner suites also passed: 67 tests / 295 assertions and
3 tests / 15 assertions, respectively.

Autoload follow-up validation: 13 autoload examples pass on MRI, C++, and LLVM.
A focused ten-file comparison covering autoload, file loading, constant lookup,
and NameError behavior matches on 487 passing examples. The LLVM backend suite
now has 69 tests / 303 assertions; the runner suite has 3 tests / 15 assertions.
