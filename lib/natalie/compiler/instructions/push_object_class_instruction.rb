require_relative './base_instruction'

module Natalie
  class Compiler
    class PushObjectClassInstruction < BaseInstruction
      def to_s
        'push_object_class'
      end

      def generate_llvm(transform)
        transform.push_call('call i64 @nat_llvm_object_class()')
      end

      def generate(transform)
        transform.push('GlobalEnv::the()->Object()')
      end
    end
  end
end
