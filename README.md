# Natalie

[![](https://github.com/seven1m/natalie/workflows/Build/badge.svg)](https://github.com/seven1m/natalie/actions?workflow=Build)

Natalie is a work-in-progress, compiled, object-oriented language resembling Ruby.

It provies an ahead-of-time compiler using C and gcc/clang as the backend. Also, the language has a REPL that performs incremental compilation.

There is much work left to do before this is useful. Please let me know if you want to help!

## Building

```sh
gem install bundler
bundle install
make build
```

## Usage

**REPL:**

```sh
bin/natalie
```

**Run a nat file:**

```sh
bin/natalie examples/hello.nat
```

**Compile a file to an executable:**

```sh
bin/natalie examples/hello.nat -c hello
./hello
```
