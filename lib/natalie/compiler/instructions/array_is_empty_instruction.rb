require_relative './base_instruction'

module Natalie
  class Compiler
    class ArrayIsEmptyInstruction < BaseInstruction
      def to_s
        'array_is_empty'
      end

      def generate(transform)
        ary = transform.peek
        transform.exec_and_push(:is_empty, "bool_object(#{ary}.as_array()->is_empty())")
      end
    end
  end
end
