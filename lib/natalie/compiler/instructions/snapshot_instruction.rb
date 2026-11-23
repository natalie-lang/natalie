require_relative './base_instruction'

module Natalie
  class Compiler
    # Freeze already-evaluated operands before a later expression can mutate
    # locals they reference. LLVM operands are SSA values and need no copies.
    class SnapshotInstruction < BaseInstruction
      def initialize(count)
        @count = count
      end

      def to_s
        "snapshot #{@count}"
      end

      def generate(transform)
        raise CompileError, 'snapshot stack underflow' if transform.stack.size < @count
        @count.times do |offset|
          index = transform.stack.size - @count + offset
          value = transform.stack[index]
          next if value.is_a?(Integer) # static argument count
          transform.stack[index] = transform.memoize(:operand, value)
        end
      end

      def generate_llvm(transform)
        raise CompileError, 'LLVM: snapshot stack underflow' if transform.stack.size < @count
      end
    end
  end
end
