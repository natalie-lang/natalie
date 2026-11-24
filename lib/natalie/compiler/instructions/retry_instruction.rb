require_relative './base_instruction'

module Natalie
  class Compiler
    class RetryInstruction < BaseInstruction
      def initialize(id:)
        @id = id
      end

      def to_s
        'retry'
      end

      def generate_llvm(transform)
        transform.emit "call void @nat_llvm_jump(ptr %env, i64 4, i32 1, i64 #{transform.break_point([:retry, @id])})"
        transform.emit 'unreachable'
        transform.emit "#{transform.name('unreachable')}:"
        transform.push('4')
      end

      private

      def retry_name
        "should_retry_#{@id}"
      end
    end
  end
end
