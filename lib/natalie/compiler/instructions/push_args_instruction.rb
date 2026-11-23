require_relative './base_instruction'

module Natalie
  class Compiler
    class PushArgsInstruction < BaseInstruction
      def initialize(for_block:, min_count:, max_count:, autosplat: false, to_array: true, include_keyword_hash: true)
        @for_block = for_block
        @min_count = min_count
        @max_count = max_count
        @autosplat = autosplat
        @to_array = to_array
        @include_keyword_hash = include_keyword_hash
      end

      def to_s
        s = "push_args #{@min_count}..#{@max_count}"
        s << ' (for_block)' if @for_block
        s << ' (autosplat)' if @autosplat
        s << ' (include_keyword_hash)' if @include_keyword_hash
        s
      end

      def generate(transform)
        if @for_block
          transform.exec_and_push(
            :args,
            "args.to_array_for_block(env, #{@min_count}, #{@max_count || -1}, #{@autosplat ? 'true' : 'false'}, #{@include_keyword_hash ? 'true' : 'false'})",
          )
        elsif @to_array
          transform.exec_and_push(:args, "args.to_array(#{@include_keyword_hash ? 'true' : 'false'})")
        else
          transform.exec_and_push(:args, 'args', type: 'auto')
        end
      end
    end
  end
end
