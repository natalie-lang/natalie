require_relative './base_instruction'

module Natalie
  class Compiler
    class PushIntInstruction < BaseInstruction
      def initialize(int)
        @int = int
      end

      attr_reader :int

      def to_s
        "push_int #{@int}"
      end

      def generate(transform)
        if @int > NAT_MAX_FIXNUM || @int < NAT_MIN_FIXNUM
          transform.push("Value::integer(#{@int.to_s.inspect})")
        else
          transform.push("Value::integer(#{@int})")
        end
      end
    end
  end
end
