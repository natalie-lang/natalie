require_relative './base_instruction'

module Natalie
  class Compiler
    class WithSingletonInstruction < BaseInstruction
      def has_body?
        true
      end

      def to_s
        'with_singleton'
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(expected_label: :with_singleton)
        fn = transform.temp('with_singleton_fn')
        transform.with_new_scope(body) do |t|
          fn_code = []
          fn_code << "Value #{fn}(Env *env, Value self) {"
          fn_code << t.transform('return')
          fn_code << '}'
          transform.top(fn_code)
        end
        obj = transform.pop
        transform.exec_and_push(
          :result_of_with_singleton,
          "#{obj}->singleton_class(env)->as_module()->eval_body(env, #{fn})"
        )
      end

      def execute(vm)
        singleton = vm.self.singleton_class
        vm.with_self(singleton) { vm.run }
        :no_halt
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
