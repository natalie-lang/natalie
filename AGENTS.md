# Natalie Agent Guide

Natalie is an ahead-of-time Ruby implementation with a C++ runtime.
`bin/natalie` is the host Ruby compiler CLI; `bin/nat` is the self-hosted compiler
after bootstrapping.

Read the relevant reference documents before making changes. Paths and commands
in these documents are relative to the repository root unless stated otherwise.

## Task Routing

- [Project structure and architecture](.agents/reference/architecture.md): compiler, runtime, libraries, and repository layout.
- [Development](.agents/reference/development.md): toolchain, build/run commands, coding style, and formatting.
- [Testing](.agents/reference/testing.md): ruby/spec, Natalie tests, focused runs, and regression coverage.
- [LLVM backend](.agents/reference/llvm.md): bridge setup, implementation entrypoints, supported features, and current boundaries.
- [Native code](.agents/reference/native-code.md): explicit C++ helpers, native methods, and retired scoped inline code.
- [Commits and pull requests](.agents/reference/contributing.md): commit conventions, review expectations, and project direction.
