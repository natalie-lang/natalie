ARG IMAGE=ruby:2.7
FROM $IMAGE

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y -q build-essential autoconf libtool clang lcov
RUN gem install bundler --no-doc

ENV LC_ALL C.UTF-8

WORKDIR '/natalie'
COPY Gemfile /natalie/Gemfile
COPY Gemfile.lock /natalie/Gemfile.lock
RUN bundle install

ARG CC=gcc
ENV CC=$CC
ARG CXX=g++
ENV CXX=$CXX

COPY ext /natalie/ext
COPY Rakefile /natalie/Rakefile

COPY bin /natalie/bin
COPY examples /natalie/examples
COPY lib /natalie/lib
COPY src /natalie/src
COPY include /natalie/include
RUN rake

COPY spec /natalie/spec
COPY test /natalie/test

ENTRYPOINT ["/natalie/bin/natalie"]
