require_relative './base_instruction'

module Natalie
  class Compiler
    class ReturnInstruction < BaseInstruction
      def to_s
        'return'
      end

      def generate_llvm(transform)
        transform.emit "ret i64 #{transform.pop}"
        transform.emit "#{transform.name('unreachable')}:"
        transform.push('4')
      end

      def generate(transform)
        value = transform.pop
        transform.exec("return #{value}")
        transform.push_nil
      end
    end
  end
end
