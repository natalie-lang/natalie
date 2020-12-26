FROM ubuntu:focal

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y -q ruby ruby-dev build-essential cmake autoconf libtool valgrind clang lcov
RUN gem install bundler --no-doc

ENV LC_ALL C.UTF-8

WORKDIR '/natalie'
COPY Gemfile /natalie/Gemfile
COPY Gemfile.lock /natalie/Gemfile.lock
RUN bundle install

ARG CC=gcc
ENV CC=$CC

COPY ext /natalie/ext
COPY CMakeLists.txt /natalie/CMakeLists.txt
COPY Makefile /natalie/Makefile

COPY bin /natalie/bin
COPY examples /natalie/examples
COPY lib /natalie/lib
COPY src /natalie/src
COPY include /natalie/include
RUN make

COPY spec /natalie/spec
COPY test /natalie/test

ENTRYPOINT ["/natalie/bin/natalie"]
