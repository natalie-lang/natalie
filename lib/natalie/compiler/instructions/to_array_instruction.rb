require_relative './base_instruction'

module Natalie
  class Compiler
    class ToArrayInstruction < BaseInstruction
      def to_s
        'to_array'
      end

      def generate(transform)
        obj = transform.pop
        transform.exec_and_push(:array, "to_ary_for_masgn(env, #{obj})")
      end

      def execute(vm)
        obj = vm.pop
        if obj.is_a?(Array)
          vm.push(obj.dup)
        elsif obj.respond_to?(:to_ary)
          vm.push(obj.to_ary.dup)
        else
          vm.push([obj])
        end
      end

      def serialize(_)
        [instruction_number].pack('C')
      end

      def self.deserialize(_, _)
        new
      end
    end
  end
end
