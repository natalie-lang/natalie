require_relative './base_instruction'

module Natalie
  class Compiler
    class CreateLambdaInstruction < BaseInstruction
      def initialize(file:, line:)
        super()

        # source location info
        @file = file
        @line = line
      end

      def to_s
        s = 'create_lambda'
        s << " (break point: #{break_point})" if break_point
        s
      end

      attr_reader :file, :line

      attr_accessor :break_point

      def generate_llvm(transform)
        transform.set_location(@file, @line)
        transform.push_call(
          "call i64 @nat_llvm_lambda(ptr %env, i64 #{transform.pop}, i64 #{@break_point ? transform.break_point(@break_point) : 0})",
        )
      end
    end
  end
end
