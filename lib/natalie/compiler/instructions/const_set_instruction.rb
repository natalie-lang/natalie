require_relative './base_instruction'

module Natalie
  class Compiler
    class ConstSetInstruction < BaseInstruction
      def initialize(name, strict:)
        @name = name.to_sym
        @strict = strict
      end

      attr_reader :name

      def to_s
        s = "const_set #{@name}"
        s << ' (strict)' if @strict
        s
      end

      def generate(transform)
        namespace = transform.pop
        value = transform.pop
        if @strict
          transform.exec("Object::const_set(env, #{namespace}, #{transform.intern(@name)}, #{value})")
        else
          transform.exec("Object::const_set_not_strict(env, #{transform.intern(@name)}, #{value})")
        end
      end
    end
  end
end
