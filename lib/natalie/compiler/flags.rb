module Natalie
  class Compiler
    module Flags
      RELEASE_FLAGS = %w[
        -pthread
        -O3
        -Wno-unused-value
        -Wno-trigraphs
      ].freeze

      DEBUG_FLAGS = %w[
        -pthread
        -g
        -Wall
        -Wextra
        -Werror
        -Wunused-result
        -Wno-missing-field-initializers
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-unused-value
        -Wno-unknown-warning-option
        -Wno-trigraphs
        -DNAT_GC_GUARD
      ].freeze

      ASAN_FLAGS = DEBUG_FLAGS + %w[
        -fsanitize=address
        -fno-omit-frame-pointer
      ]

      COVERAGE_FLAGS = DEBUG_FLAGS + %w[
        -fprofile-arcs
        -ftest-coverage
      ].freeze
    end
  end
end
