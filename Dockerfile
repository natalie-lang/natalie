ARG IMAGE=ruby:2.7
FROM $IMAGE

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y -q build-essential autoconf libtool clang lcov
RUN gem install bundler --no-doc

ENV LC_ALL C.UTF-8

WORKDIR natalie
COPY .git/ .git/
COPY .gitmodules .gitmodules
RUN git submodule update --init

COPY Gemfile Gemfile.lock /natalie/ 
RUN bundle install

ARG CC=gcc
ENV CC=$CC
ARG CXX=g++
ENV CXX=$CXX

COPY ext ext
COPY Rakefile Rakefile

COPY bin bin
COPY examples examples
COPY lib lib
COPY src src
COPY include include
RUN rake

COPY spec spec
COPY test test

ENTRYPOINT ["/natalie/bin/natalie"]
