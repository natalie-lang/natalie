# Development

## Build and Run

Use `nix develop` for the configured toolchain, or follow the README's manual setup. Match `.ruby-version` when selecting host Ruby.

- `bundle install`: install Ruby dependencies.
- `bundle exec rake`: build Natalie and dependencies.
- `bundle exec rake build_debug`: build with debugging and stricter warnings.
- `bin/natalie examples/hello.rb`: compile and run a script.
- `bin/natalie -c hello examples/hello.rb`: produce an executable.

For LLVM setup and commands, see [LLVM backend](llvm.md).

## Coding Style and Naming

Use two-space indentation for Ruby and four spaces for C/C++. Follow `.editorconfig`, `.clang-format`, and `.streerc`; Ruby formatting uses Syntax Tree with a 120-column width. Run `bundle exec rake format`, reviewing the diff for unrelated changes. Format `tools/llvm_runtime.cpp` explicitly with `clang-format` when editing it.

Use snake_case Ruby filenames and methods, CamelCase classes, and existing neighboring C++ conventions. Keep instruction-specific backend generation in the instruction classes.

Formatting CI and the commit hook use the toolchain pinned by `flake.lock`.
Run formatting through `nix develop -c bundle exec rake format` to use the same
clang-format and Syntax Tree versions. Check the C++ formatter version with
`nix develop -c clang-format --version`; avoid the system formatter when it differs.
