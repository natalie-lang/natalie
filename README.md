# Natalie

[![github build status](https://github.com/seven1m/natalie/workflows/Build/badge.svg)](https://github.com/seven1m/natalie/actions?workflow=Build)
[![builds.sr.ht status](https://builds.sr.ht/~tim/natalie.svg)](https://builds.sr.ht/~tim/natalie?)

Natalie is a very early-stage work-in-progress Ruby implementation.

It provides an ahead-of-time compiler using C++ and gcc/clang as the backend.
Also, the language has a REPL that performs incremental compilation.

![demo screencast](examples/demo.gif)

There is much work left to do before this is useful. Please let me know if you
want to help!

## Helping Out

Contributions are welcome! You can learn more about how I work on Natalie via
the [hacking session videos on YouTube](https://www.youtube.com/playlist?list=PLWUx_XkUoGTq-nkbhnk6PN4m109ISo5BX).

The easiest way to get started right now would be to find a method on an object
that is not yet implemented and make it yourself! Also take a look at
[good first issues](https://github.com/seven1m/natalie/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22).

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
cd natalie
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
docker run -it --rm -v$(pwd)/myfile.rb:/myfile.rb natalie /myfile.rb # execute a local rb file
docker run -it --rm --entrypoint bash natalie                        # bash prompt
```

## Copyright & License

Natalie is copyright 2021, Tim Morgan and contributors. Natalie is licensed
under the MIT License; see the `LICENSE` file in this directory for the full text.

Some parts of this program are copied from other sources, and the copyright
belongs to the respective owner:

| file(s)                       | copyright                         | license           |
|-------------------------------|-----------------------------------|-------------------|
| `dtoa.c`                      | David M. Gay, Lucent Technologies | custom permissive |
| `fiber_value.*`               | Evan Jones                        | MIT               |
| `hashmap.*`                   | David Leeds                       | MIT               |
| `spec/*` (see `spec/LICENSE`) | Engine Yard, Inc.                 | MIT               |

See each file above for full copyright and license text.
