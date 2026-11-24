require_relative './base_instruction'

module Natalie
  class Compiler
    # Compose definition probes without evaluating the argument expressions.
    class CheckDefinedInstruction < BaseInstruction
      def to_s
        'check_defined'
      end

      def generate_llvm(transform)
        transform.emit "call void @nat_llvm_check_defined(i64 #{transform.pop})"
      end
    end
  end
end
