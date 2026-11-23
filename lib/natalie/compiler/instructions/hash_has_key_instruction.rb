require_relative './base_instruction'

module Natalie
  class Compiler
    class HashHasKeyInstruction < BaseInstruction
      def initialize(key)
        raise "expected symbol but got: #{key.inspect}" unless key.is_a?(Symbol)

        @key = key
      end

      def to_s
        "hash_has_key #{@key.inspect}"
      end

      def generate(transform)
        hash = transform.peek
        transform.exec_and_push(:has_key, "bool_object(#{hash}.as_hash()->has_key(env, #{@key.to_s.inspect}_s))")
      end
    end
  end
end
