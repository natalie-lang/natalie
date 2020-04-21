FROM ubuntu:focal

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y -q ruby ruby-dev build-essential cmake autoconf libtool valgrind clang lcov
RUN gem update --system -q --silent
RUN gem install bundler --no-doc

ENV LC_ALL C.UTF-8

WORKDIR '/natalie'
COPY Gemfile /natalie/Gemfile
COPY Gemfile.lock /natalie/Gemfile.lock
RUN bundle install

ARG CC=gcc
ENV CC=$CC

COPY ext /natalie/ext
COPY Makefile /natalie/Makefile
RUN make ext/onigmo/.libs/libonigmo.a

COPY bin /natalie/bin
COPY examples /natalie/examples
COPY lib /natalie/lib
COPY src /natalie/src
RUN mkdir -p obj/nat
RUN make build

COPY spec /natalie/spec
COPY test /natalie/test

ENTRYPOINT ["/natalie/bin/natalie"]
