require_relative './base_instruction'

module Natalie
  class Compiler
    class PatternArraySizeCheckInstruction < BaseInstruction
      def initialize(length:)
        super()
        @length = length
      end

      attr_reader :length

      def to_s
        "pattern_array_size_check #{@length}"
      end

      def generate(transform)
        obj = transform.peek
        array_size = transform.temp('array_size')
        transform.exec("auto #{array_size} = #{obj}->as_array()->size()")
        transform.exec(
          "if (#{@length} > #{array_size} || #{array_size} > #{@length}) " \
          "env->raise(\"StandardError\", \"length mismatch (given {}, expected #{@length.inspect})\", #{array_size})"
        )
      end

      def execute(_vm)
        raise 'todo'
      end
    end
  end
end
