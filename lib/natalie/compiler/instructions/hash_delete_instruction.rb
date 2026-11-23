require_relative './base_instruction'

module Natalie
  class Compiler
    class HashDeleteInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      def to_s
        "hash_delete #{@name}"
      end

      def generate(transform)
        hash = transform.peek
        transform.exec_and_push(@name, "#{hash}.as_hash()->remove(env, #{transform.intern(@name)}).value()")
      end
    end
  end
end
