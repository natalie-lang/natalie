# Natalie

[![](https://github.com/seven1m/natalie/workflows/Build/badge.svg)](https://github.com/seven1m/natalie/actions?workflow=Build)

Natalie is a work-in-progress Ruby implementation.

It provies an ahead-of-time compiler using C and gcc/clang as the backend.
Also, the language has a REPL that performs incremental compilation.

There is much work left to do before this is useful. Please let me know if you
want to help!

## Current Status

Natalie is able to compile and run **very simple Ruby** scripts at the moment.
We have the foundations in place, like classes, modules, methods, blocks and
the various types of variables. We have the basic data objects in place, e.g.
true, false, nil, Integer, Symbol, String, Array, Hash, Range.  But there are
still some yet unimplemented, like Float, and probably 90% of convenience
methods on all the various objects.

Here is a rough to do list (not in any specific order):

- encoding support for string and regex (just UTF-8 would be great!)
- Keyword arguments and a bunch of other method argument handling
- Float
- Bignum support
- A garbage collector (I'm debating on whether to pull in the
  [Boehm](https://www.hboehm.info/gc/) collector or to roll our own)
- Lots and lots of convenience methods on built-in objects
- A standard library (Hopefully we can pull most of it in from MRI once the
  core is more complete)
- MRI C-extension compatibility
- Probably lots more I just haven't thought of yet!

The current driving force is to make Natalie complete enough to be able to
compile itself. Since the Natalie compiler is written in Ruby, theoretically
at some point, we should be able to run the Ruby code through Natalie to
produce a compiled version of the compiler. That day will be glorious!

A more lofty goal would be the ability to compile a larger project like
Sinatra. But that's probably a year away at this point.

## Helping Out

Contributions are welcome! You can learn more about how I work on Natalie via
the [hacking session videos on YouTube](https://www.youtube.com/playlist?list=PLWUx_XkUoGTq-nkbhnk6PN4m109ISo5BX).

The easiest way to get started right now would be to find a method on an object
that is not yet implemented and make it yourself!

## Building

The compiler and REPL currently (but hopefully not for long!) require Ruby.
I've only tested on Ruby 2.6.x.

```sh
git submodule update --init
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

## Using With Docker

```
docker build -t natalie .                                               # build image
docker run -it --rm natalie                                             # repl
docker run -it --rm natalie -e "p 2 * 3"                                # immediate
docker run -it --rm -v$(pwd)/myfile.nat:/myfile.nat natalie /myfile.nat # execute a local nat file
docker run -it --rm --entrypoint bash natalie                           # bash prompt
```
