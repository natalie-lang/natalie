require_relative './base_instruction'

module Natalie
  class Compiler
    class WithSelfInstruction < BaseInstruction
      def has_body?
        true
      end

      def to_s
        'with_self'
      end

      def generate(transform)
        new_self = transform.pop
        body = transform.fetch_block_of_instructions(expected_label: :with_self)
        old_self = transform.temp('old_self')
        result = transform.temp('with_self_result')
        code = []
        transform.with_same_scope(body) do |t|
          code << "auto #{old_self} = self"
          code << "self = #{new_self}"
          code << t.transform("auto #{result} = ")
          code << "self = #{old_self}"
        end
        transform.exec(code)
        transform.push(result)
      end

      def execute(vm)
        new_self = vm.pop
        vm.with_self(new_self) { vm.run }
        :no_halt
      end
    end
  end
end
