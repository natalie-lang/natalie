require_relative './base_instruction'

module Natalie
  class Compiler2
    class WithSelfInstruction < BaseInstruction
      def has_body?
        true
      end

      def to_s
        'with_self'
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(expected_label: :with_self)
        fn = transform.temp('with_self_fn')
        transform.with_new_scope(body) do |t|
          fn_code = []
          fn_code << "Value #{fn}(Env *env, Value self) {"
          fn_code << t.transform('return')
          fn_code << '}'
          transform.top(fn_code)
        end
        new_self = transform.pop
        transform.exec_and_push(:result_of_with_self, "#{new_self}->as_module()->eval_body(env, #{fn})")
      end

      def execute(vm)
        new_self = vm.pop
        vm.with_self(new_self) { vm.run }
      end
    end
  end
end
