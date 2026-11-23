require_relative './base_instruction'

module Natalie
  class Compiler
    class ArgsShiftInstruction < BaseInstruction
      def initialize(include_keyword_hash:)
        @include_keyword_hash = include_keyword_hash
      end

      def to_s
        s = 'args_shift'
        s << ' (include_keyword_hash)' if @include_keyword_hash
        s
      end

      def generate_llvm(transform)
        transform.push_call("call i64 @nat_llvm_args_shift(ptr %env, ptr %args, i32 #{@include_keyword_hash ? 1 : 0})")
      end

      def generate(transform)
        transform.exec_and_push(:first_arg, "args.shift(env, #{@include_keyword_hash ? 'true' : 'false'})")
      end
    end
  end
end
