module Natalie
  class Compiler
    module Flags
      RELEASE_FLAGS = %w[
        -pthread
        -O3
        -Wno-unused-value
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
        -DNAT_GC_GUARD
      ].freeze

      COVERAGE_FLAGS = DEBUG_FLAGS + %w[
        -fprofile-arcs
        -ftest-coverage
      ].freeze
    end
  end
end
