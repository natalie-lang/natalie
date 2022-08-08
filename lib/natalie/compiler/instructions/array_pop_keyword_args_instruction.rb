require_relative './base_instruction'

module Natalie
  class Compiler
    # before:
    #   args = [1, 2, { a: 'b' }]
    #   stack: [args]
    #
    # after:
    #   args = [1, 2]
    #   stack: [args, { a: 'b' }]
    #
    # If the array has no keyword args hash, then an
    # empty hash is pushed to the stack instead.
    class ArrayPopKeywordArgsInstruction < BaseInstruction
      def to_s
        'array_pop_keyword_args'
      end

      def generate(transform)
        ary = transform.memoize(:ary, transform.peek)
        code = "args.has_keyword_hash() ? #{ary}->as_array()->pop() : new HashObject"
        transform.exec_and_push(:keyword_args_hash, code)
      end

      def execute(vm)
        ary = vm.peek
        if ary.empty? || !ary.last.is_a?(Hash)
          vm.push Hash.new
        else
          vm.push ary.pop
        end
      end
    end
  end
end
