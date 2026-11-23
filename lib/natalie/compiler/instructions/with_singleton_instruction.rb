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

      def generate_llvm(transform)
        body = transform.fetch_block_of_instructions(expected_label: :with_singleton)
        fn = transform.function(body, scope: env)
        transform.push_call("call i64 @nat_llvm_with_singleton(ptr %env, i64 #{transform.pop}, ptr #{fn})")
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(expected_label: :with_singleton)
        fn = transform.temp('with_singleton_fn')
        transform.with_new_scope(body) do |t|
          fn_code = []
          fn_code << "Value #{fn}(Env *env, Value self) {"
          fn_code << t.transform('return')
          fn_code << '}'
          transform.top(fn, fn_code)
        end
        obj = transform.pop

        singleton_class = transform.temp('singleton_class')

        code = []
        code << "Value #{singleton_class} = Object::singleton_class(env, #{obj});"
        code << "#{singleton_class}.as_class()->eval_body(env, #{fn});"

        transform.exec_and_push(:result_of_with_singleton, code)
      end
    end
  end
end
