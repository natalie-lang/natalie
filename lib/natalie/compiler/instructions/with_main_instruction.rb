require_relative './base_instruction'

module Natalie
  class Compiler
    class WithMainInstruction < BaseInstruction
      def has_body?
        true
      end

      def to_s
        'with_main'
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(expected_label: :with_main)
        old_self = transform.temp('old_self')
        result = transform.temp('with_main_result')
        code = []
        transform.with_same_scope(body) do |t|
          code << "auto #{old_self} = self"
          code << 'self = GlobalEnv::the()->main_obj()'
          code << t.transform("auto #{result} = ")
          code << "self = #{old_self}"
        end
        transform.exec(code)
        transform.push(result)
      end

      def execute(vm)
        vm.with_self(vm.main) { vm.run }
        :no_halt
      end
    end
  end
end
