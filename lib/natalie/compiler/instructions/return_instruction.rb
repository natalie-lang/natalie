require_relative './base_instruction'

module Natalie
  class Compiler
    class ReturnInstruction < BaseInstruction
      def to_s
        'return'
      end

      def generate(transform)
        value = transform.pop
        if return_env.fetch(:outer).nil?
          # return from top-level EVAL() function must return Object*
          transform.exec("return #{value}.object()")
        else
          transform.exec("return #{value}")
        end
        transform.push('Value(NilObject::the())')
      end

      def execute(_vm)
        :return
      end

      private

      def return_env
        env = @env
        env = env.fetch(:outer) while env[:hoist]
        env
      end
    end
  end
end
