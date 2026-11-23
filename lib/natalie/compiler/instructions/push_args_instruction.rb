require_relative './base_instruction'

module Natalie
  class Compiler
    class PushArgsInstruction < BaseInstruction
      def initialize(for_block:, min_count:, max_count:, autosplat: false, include_keyword_hash: true)
        @for_block = for_block
        @min_count = min_count
        @max_count = max_count
        @autosplat = autosplat
        @include_keyword_hash = include_keyword_hash
      end

      def to_s
        s = "push_args #{@min_count}..#{@max_count}"
        s << ' (for_block)' if @for_block
        s << ' (autosplat)' if @autosplat
        s << ' (include_keyword_hash)' if @include_keyword_hash
        s
      end

      def generate_llvm(transform)
        transform.push_call(
          "call i64 @nat_llvm_args(ptr %env, ptr %args, i32 #{@for_block ? 1 : 0}, i64 #{@min_count}, i64 #{@max_count || -1}, i32 #{@autosplat ? 1 : 0}, i32 #{@include_keyword_hash ? 1 : 0})",
        )
      end

      def generate(transform)
        if @for_block
          transform.exec_and_push(
            :args,
            "args.to_array_for_block(env, #{@min_count}, #{@max_count || -1}, #{@autosplat ? 'true' : 'false'}, #{@include_keyword_hash ? 'true' : 'false'})",
          )
        else
          transform.exec_and_push(:args, "args.to_array(#{@include_keyword_hash ? 'true' : 'false'})")
        end
      end
    end
  end
end
