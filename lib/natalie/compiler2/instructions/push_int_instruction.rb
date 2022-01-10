require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushIntInstruction < BaseInstruction
      def initialize(int)
        @int = int
      end

      attr_reader :int

      def to_s
        "push_int #{@int}"
      end

      def to_cpp(transform)
        "Value::integer(#{@int})"
      end
    end
  end
end
