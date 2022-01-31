require_relative './base_instruction'

module Natalie
  class Compiler2
    class HashDeleteInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      def to_s
        "hash_delete #{@name}"
      end

      def generate(transform)
        ary = transform.peek
        transform.exec_and_push(:value_removed_from_hash, "#{ary}->as_hash()->remove(env, #{@name.to_s.inspect}_s)")
      end

      def execute(vm)
        hash = vm.peek
        vm.push(hash.delete(@name))
      end
    end
  end
end
