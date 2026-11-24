require_relative './base_instruction'

module Natalie
  class Compiler
    class DupRelInstruction < BaseInstruction
      # offset = 0 is equivalent to DupInstruction
      def initialize(offset)
        @offset = offset
      end

      def to_s
        "dup_rel #{@offset}"
      end

      def generate_llvm(transform)
        raise CompileError, 'LLVM: instruction stack underflow' unless transform.stack.size > @offset
        transform.push(transform.stack[-1 - @offset])
      end
    end
  end
end
