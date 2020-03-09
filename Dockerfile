FROM ubuntu:focal

RUN apt-get update && apt-get install -y -q ruby ruby-dev build-essential cmake autoconf libtool valgrind clang lcov
RUN gem update --system -q --silent
RUN gem install bundler --no-doc

WORKDIR '/natalie'
COPY Gemfile /natalie/Gemfile
COPY Gemfile.lock /natalie/Gemfile.lock
RUN bundle install

COPY . /natalie
RUN mkdir -p obj/nat
RUN make clean build

ENTRYPOINT ["/natalie/bin/natalie"]
