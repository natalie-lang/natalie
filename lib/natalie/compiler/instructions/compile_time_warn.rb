# frozen_string_literal: true

require_relative 'base_instruction'

module Natalie
  class Compiler
    class CompileTimeWarn < BaseInstruction
      attr_reader :message

      def initialize(message, level:)
        @message = message
        @level = level
      end

      def to_s
        level_str = @level && @level != :default ? " (level=#{@level})" : ''
        "compile_time_warn: #{message}#{level_str}"
      end

      def generate_llvm(transform)
        transform.emit "call void @nat_llvm_warn(ptr %env, i64 %self, ptr #{transform.string(message)}, i32 #{@level == :verbose ? 1 : 0})"
      end
    end
  end
end
