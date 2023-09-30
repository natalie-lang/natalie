require_relative './base_instruction'

module Natalie
  class Compiler
    class AutoloadConstInstruction < BaseInstruction
      def initialize(name)
        @name = name
      end

      def has_body?
        true
      end

      def to_s
        "autoload_const #{@name}"
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(expected_label: :autoload_const)
        fn = transform.temp("autoload_const_#{@name}_fn")
        transform.with_new_scope(body) do |t|
          fn_code = []
          fn_code << "Value #{fn}(Env *env, Value self, Args args = {}, Block *block = nullptr) {"
          fn_code << t.transform('return')
          fn_code << '}'
          transform.top(fn_code)
        end
        transform.exec("self->const_set(#{transform.intern(@name)}, #{fn})")
        transform.push_nil
      end

      def execute(vm)
        raise 'todo'
      end
    end
  end
end
