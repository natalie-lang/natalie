require_relative './base_instruction'

module Natalie
  class Compiler
    class TryInstruction < BaseInstruction
      attr_reader :serial, :has_ensure

      def initialize(discard_catch_result: false, has_ensure: false)
        @discard_catch_result = discard_catch_result
        @has_ensure = has_ensure
        @serial = self.class.serial
      end

      def to_s
        s = 'try'
        s << ' (discard_catch_result)' if @discard_catch_result
        s << ' (ensure)' if @has_ensure
        s
      end

      def has_body?
        true
      end

      def generate_llvm(transform)
        body = transform.fetch_block_of_instructions(until_instruction: CatchInstruction)
        handler = transform.fetch_block_of_instructions(expected_label: :try)
        baseline = transform.stack.dup
        protected_fn, protected_size = transform.region(body, baseline)
        catch_fn, catch_size = transform.region(handler, baseline)
        frame = transform.argument_array(baseline)
        transform.stack.replace(baseline.take([protected_size, catch_size].min))
        transform.push_call(
          "call i64 @nat_llvm_try(ptr %env, i64 %self, ptr #{transform.function_args}, ptr #{transform.function_block}, ptr #{protected_fn}, ptr #{catch_fn}, ptr #{frame}, i32 #{@has_ensure ? 1 : 0}, i32 #{@discard_catch_result ? 1 : 0}, i64 #{transform.break_point([:retry, @serial])})",
        )
      end

      @serial = 0

      def self.serial
        @serial += 1
      end
    end
  end
end
