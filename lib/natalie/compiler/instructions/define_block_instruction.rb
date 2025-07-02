require_relative './base_instruction'

module Natalie
  class Compiler
    class DefineBlockInstruction < BaseInstruction
      def initialize(arity:, for_lambda: false, has_return: false)
        @arity = arity
        @for_lambda = for_lambda
        @has_return = has_return
      end

      def has_body?
        true
      end

      def for_lambda? = !!@for_lambda
      def has_return? = !!@has_return

      attr_reader :arity
      attr_accessor :has_return

      def to_s
        s = 'define_block'
        s << ' (for_lambda)' if for_lambda?
        s << ' (has_return)' if has_return?
        s
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(expected_label: :define_block)
        fn = transform.temp('block')
        transform.with_new_scope(body) do |t|
          body = []
          body << "Value #{fn}(Env *env, Value self, Args &&args, Block *block) {"
          body << t.transform('return')
          body << '}'
          transform.top(fn, body)
        end
        transform.push("Block::create(*env, self, #{fn}, #{@arity}, #{has_return?})")
      end

      def execute(vm)
        vm.push(Natalie::VM::Block.new(vm: vm))
        vm.skip_block_of_instructions(expected_label: :define_block)
        nil
      end

      def serialize(_)
        [instruction_number, arity, for_lambda? ? 1 : 0, has_return? ? 1 : 0].pack('CwCC')
      end

      def self.deserialize(io, _)
        arity = io.read_ber_integer
        for_lambda = io.getbool
        has_return = io.getbool
        new(arity:, for_lambda:, has_return:)
      end
    end
  end
end
