require_relative './base_instruction'

module Natalie
  class Compiler
    class PushObjectClassInstruction < BaseInstruction
      def to_s
        'push_object_class'
      end

      def generate(transform)
        transform.push('GlobalEnv::the()->Object()')
      end
    end
  end
end
