require_relative './base_instruction'

module Natalie
  class Compiler
    class UndefineMethodInstruction < BaseInstruction
      def initialize(name:)
        @name = name
      end

      attr_reader :name

      def to_s
        "undefine_method #{@name}"
      end

      def generate(transform)
        transform.exec("self->undefine_method(env, #{transform.intern(@name)})")
      end

      def execute(vm)
        raise 'todo'
      end
    end
  end
end
