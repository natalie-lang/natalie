require_relative './base_instruction'

module Natalie
  class Compiler
    class NotInstruction < BaseInstruction
      def to_s
        'not'
      end

      def generate_llvm(transform)
        transform.push(0)
        SendInstruction.new(
          :!,
          receiver_is_self: false,
          with_block: false,
          file: transform.source_path,
          line: 1,
        ).generate_llvm(transform)
      end
    end
  end
end
