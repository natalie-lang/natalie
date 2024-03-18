require_relative './base_instruction'

module Natalie
  class Compiler
    class ArrayShiftInstruction < BaseInstruction
      def to_s
        'array_shift'
      end

      def generate(transform)
        ary = transform.peek
        transform.exec_and_push(:first_item_of_array, "#{ary}->as_array()->shift()")
      end

      def execute(vm)
        ary = vm.peek
        unless ary.is_a?(Array)
          ary = Array(vm.pop)
          vm.push(ary)
        end
        vm.push(ary.shift)
      end

      def serialize
        [instruction_number].pack('C')
      end

      def self.deserialize(_)
        new
      end
    end
  end
end
