require_relative './base_instruction'

module Natalie
  class Compiler
    class ArgsEmptyInstruction < BaseInstruction
      def initialize(include_keyword_hash:)
        @include_keyword_hash = include_keyword_hash
      end

      def to_s
        s = 'args_empty'
        s << ' (include_keyword_hash)' if @include_keyword_hash
        s
      end

      def generate(transform)
        transform.exec_and_push(:args_empty, "bool_object(args.size(#{@include_keyword_hash}) == 0)")
      end
    end
  end
end
