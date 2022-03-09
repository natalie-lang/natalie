require_relative './base_instruction'

module Natalie
  class Compiler2
    class ArrayShiftInstruction < BaseInstruction
      def to_s
        'array_shift'
      end

      def generate(transform)
        ary = transform.peek
        unless ary =~ /^array/
          transform.exec_and_push(:array, "to_ary(env, #{transform.pop}, true)")
          ary = transform.peek
        end
        transform.exec_and_push(:first_item_of_array, "#{ary}->shift()")
      end

      def execute(vm)
        ary = vm.peek
        unless ary.is_a?(Array)
          ary = Array(vm.pop)
          vm.push(ary)
        end
        vm.push(ary.shift)
      end
    end
  end
end
