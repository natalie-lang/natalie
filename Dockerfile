ARG IMAGE=ruby:3.3
FROM $IMAGE

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y -q build-essential autoconf libtool clang lcov clang-tidy libclang-dev lldb gdb python3 python3-pip ccache

# PIP externally-managed-environment now forces you
# to be explicit when installing system-wide package
# https://veronneau.org/python-311-pip-and-breaking-system-packages.html
RUN pip3 install compiledb || pip3 install compiledb --break-system-packages
RUN gem install bundler --no-doc

# Valgrind on our docker image is too old to handle compressed debug symbols or some such.
ARG NEED_VALGRIND=false
RUN if [ "$NEED_VALGRIND" = "true" ]; then \
  cd /tmp && \
  wget https://sourceware.org/pub/valgrind/valgrind-3.24.0.tar.bz2 && \
  tar xvf valgrind-3.24.0.tar.bz2 && \
  cd valgrind-3.24.0 && \
  ./configure && \
  make -j 4 && \
  make install; \
  fi

ENV LC_ALL=C.UTF-8
ENV LLVM_CONFIG=/usr/lib/llvm-14/bin/llvm-config

WORKDIR /natalie
COPY .git/ .git/
COPY .gitmodules .gitmodules
COPY .clang-tidy .clang-tidy
RUN git submodule update --init --recursive

COPY Gemfile Gemfile.lock /natalie/
RUN bundle install

ARG CC=gcc
ENV CC=$CC
ARG CXX=g++
ENV CXX=$CXX

ARG NAT_CXX_FLAGS
ENV NAT_CXX_FLAGS=$NAT_CXX_FLAGS

ENV USER=root

COPY ext ext
COPY Rakefile Rakefile

COPY bin bin
COPY examples examples
COPY lib lib
COPY src src
COPY include include

ARG NAT_BUILD_MODE=release
RUN rake build_${NAT_BUILD_MODE}

COPY spec spec
COPY test test

ARG BUILD_TEST_SUPPORT=false
RUN if [ "$BUILD_TEST_SUPPORT" = "true" ]; then \
  rake build_test_support; \
  fi

ENTRYPOINT ["/natalie/bin/natalie"]
