require_relative './base_instruction'

module Natalie
  class Compiler
    class OnceInstruction < BaseInstruction
      def to_s
        'once'
      end

      def has_body?
        true
      end

      def generate_llvm(transform)
        body = transform.fetch_block_of_instructions(expected_label: :once)
        baseline = transform.stack.dup
        fn, size = transform.region(body, baseline)
        frame = transform.argument_array(baseline)
        transform.stack.replace(baseline.take(size))
        cache = transform.literal_cache([:once, self])
        transform.push_call(
          "call i64 @nat_llvm_once(ptr %env, i64 %self, ptr #{transform.function_args}, ptr #{transform.function_block}, ptr #{cache}, ptr #{fn}, ptr #{frame})",
        )
      end
    end
  end
end
