# Repository Guidelines

## Project Structure & Module Organization

Natalie is an ahead-of-time Ruby implementation with a C++ runtime.

- `bin/natalie`: host Ruby compiler CLI; `bin/nat`: self-hosted compiler after bootstrapping.
- `lib/natalie/compiler/`: compiler passes, instructions, and backends. Instructions implement `generate` for C++ and `generate_llvm` for the experimental LLVM backend.
- `src/` and `include/natalie/`: runtime implementation, headers, and generated-code templates.
- `lib/`: Ruby standard-library implementations; `ext/`: bundled dependencies.
- `spec/`: imported ruby/spec conformance tests; `test/`: Natalie-specific tests and host Ruby integration tests.
- `examples/`: sample programs and assets; `tools/`: development utilities and LLVM runtime bridge; `build/`: generated artifacts.

## Build, Test, and Development Commands

Use `nix develop` for the configured toolchain, or follow the README's manual setup. Match `.ruby-version` when selecting host Ruby.

- `bundle install`: install Ruby dependencies.
- `bundle exec rake`: build Natalie and dependencies.
- `bundle exec rake build_debug`: build with debugging and stricter warnings.
- `bin/natalie examples/hello.rb`: compile and run a script.
- `bin/natalie -c hello examples/hello.rb`: produce an executable.
- `bundle exec rake test`: build and run the expected-passing suite.
- `bundle exec rake llvm_runtime`: build the optional LLVM bridge after the normal build.
- `bundle exec rake test_llvm`: run LLVM backend tests; requires a shared LLVM library.

## Coding Style & Naming Conventions

Use two-space indentation for Ruby and four spaces for C/C++. Follow `.editorconfig`, `.clang-format`, and `.streerc`; Ruby formatting uses Syntax Tree with a 120-column width. Run `bundle exec rake format`, reviewing the diff for unrelated changes. Format `tools/llvm_runtime.cpp` explicitly with `clang-format` when editing it.

Use snake_case Ruby filenames and methods, CamelCase classes, and existing neighboring C++ conventions. Keep instruction-specific backend generation in the instruction classes.

## Testing Guidelines

Prefer imported ruby/spec coverage that matches MRI behavior. Use `NATFIXME` for known unsupported cases instead of commenting out tests. Name specs `*_spec.rb` and custom tests `*_test.rb`. Host integration tests use Minitest.

Run focused tests with `bin/natalie spec/core/string/strip_spec.rb` or `bundle exec ruby test/ruby/argv_test.rb`. Add relevant regression coverage; no numeric coverage threshold is documented.

## Commit & Pull Request Guidelines

Follow CONTRIBUTING.md: make atomic commits with capitalized, imperative subjects, and wrap messages at 72 characters. History also contains descriptive subjects and automated formatting commits.

Describe the problem, resulting behavior, and validation in PRs; reference relevant issues. Discuss uncertain project-direction changes in an issue first. Squash review revisions before merging.
