# Natalie

[![github build status](https://github.com/seven1m/natalie/actions/workflows/build.yml/badge.svg)](https://github.com/seven1m/natalie/actions?query=workflow%3ABuild+branch%3Amaster)
[![builds.sr.ht status](https://builds.sr.ht/~tim/natalie.svg)](https://builds.sr.ht/~tim/natalie?)
[![MIT License](https://img.shields.io/badge/license-MIT-blue)](https://github.com/seven1m/natalie/blob/master/LICENSE)

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

We have a very quiet [Discord server](https://discord.gg/hnHp2tdQyn) -- come and hang out!

## Building

Natalie is tested on macOS, OpenBSD, and Ubuntu Linux. Windows is not yet supported.

The compiler and REPL require MRI Ruby. I only test with Ruby 2.7.x currently.

Prerequisites:

- git
- autoconf
- automake
- libtool
- make
- gcc or clang
- Ruby 2.7.x or Ruby 3.0.x
- ccache (optional)

Install the above prerequisites on your platform, then run:

```sh
git clone https://github.com/seven1m/natalie
cd natalie
git submodule update --init
gem install bundler
bundle install
rake
```

**NOTE:** Currently, the default build is the "debug" build, since Nataile is in active development.
But you can build in release mode with `rake build_release`.

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
| ----------------------------- | --------------------------------- | ----------------- |
| `dtoa.c`                      | David M. Gay, Lucent Technologies | custom permissive |
| `fiber_value.*`               | Evan Jones                        | MIT               |
| `bignum/*`                    | Micheal Clark                     | MIT               |
| `spec/*` (see `spec/LICENSE`) | Engine Yard, Inc.                 | MIT               |

See each file above for full copyright and license text.
