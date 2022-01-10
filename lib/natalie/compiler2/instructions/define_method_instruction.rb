require_relative './base_instruction'

module Natalie
  class Compiler2
    class DefineMethodInstruction < BaseInstruction
      def initialize(name:, arity:)
        @name = name
        @arity = arity
      end

      def has_body?
        true
      end

      attr_reader :name, :arity

      def to_s
        "define_method #{@name}"
      end

      def to_cpp(transform)
        body = transform.fetch_block_of_instructions(expected_label: :define_method)
        fn = transform.temp('fn')
        transform.with_new_scope(body) do |t|
          transform.top "Value #{fn}(Env *env, Value self, size_t argc, Value *args, Block *block) {"
          transform.top t.transform('return')
          transform.top '}'
        end
        "self->define_method(env, #{@name.to_s.inspect}_s, #{fn}, #{@arity})"
      end
    end
  end
end
