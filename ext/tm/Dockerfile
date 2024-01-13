ARG IMAGE=ruby:3.1
FROM $IMAGE

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y -q build-essential clang libclang-dev
RUN gem install bundler --no-doc

ENV LC_ALL C.UTF-8

WORKDIR tm

COPY Gemfile /tm/ 
RUN bundle install

ARG CC=gcc
ENV CC=$CC
ARG CXX=g++
ENV CXX=$CXX

ENV LLVM_CONFIG=/usr/lib/llvm-14/bin/llvm-config

COPY Rakefile Rakefile
COPY include include
COPY test test
