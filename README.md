# Natalie

[![github build status](https://github.com/seven1m/natalie/workflows/Build/badge.svg)](https://github.com/seven1m/natalie/actions?workflow=Build)
[![builds.sr.ht status](https://builds.sr.ht/~tim/natalie.svg)](https://builds.sr.ht/~tim/natalie?)

Natalie is a very early-stage work-in-progress Ruby implementation.

It provides an ahead-of-time compiler using C++ and gcc/clang as the backend.
Also, the language has a REPL that performs incremental compilation.

There is much work left to do before this is useful. Please let me know if you
want to help!

## Helping Out

Contributions are welcome! You can learn more about how I work on Natalie via
the [hacking session videos on YouTube](https://www.youtube.com/playlist?list=PLWUx_XkUoGTq-nkbhnk6PN4m109ISo5BX).

The easiest way to get started right now would be to find a method on an object
that is not yet implemented and make it yourself!

## Building

Natalie is tested on macOS, OpenBSD, and Ubuntu Linux. Windows is not yet supported.

The compiler and REPL require MRI Ruby. I only test with Ruby 2.7.x currently.

Prerequisites:

- git
- autoconf
- automake
- libtool
- cmake
- GNU make
- Ruby 2.7.x

Install the above prerequisites on your platform, then run:

```sh
git clone https://github.com/seven1m/natalie
git submodule update --init
gem install bundler
bundle install
make # or gmake if your system make isn't GNU make
```

**NOTE:** Currently, the default build is the "debug" build.

## Usage

**REPL:**

```sh
bin/natalie
```

**Run a Ruby script:**

```sh
bin/natalie examples/hello.rb
```

**Compile a file to an executable:**

```sh
bin/natalie examples/hello.rb -c hello
./hello
```

## Using With Docker

```
docker build -t natalie .                                            # build image
docker run -it --rm natalie                                          # repl
docker run -it --rm natalie -e "p 2 * 3"                             # immediate
docker run -it --rm -v$(pwd)/myfile.rb:/myfile.rb natalie /myfile.rb # execute a local nat file
docker run -it --rm --entrypoint bash natalie                        # bash prompt
```
