require_relative './base_instruction'

module Natalie
  class Compiler2
    class ReraiseInstruction < BaseInstruction
      def to_s
        'reraise'
      end

      def generate(transform)
        transform.exec('env->raise_exception(exception)')
        transform.push('NilObject::the()')
      end

      def execute(_)
        raise
      end
    end
  end
end
