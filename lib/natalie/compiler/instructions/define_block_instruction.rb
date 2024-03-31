require_relative './base_instruction'

module Natalie
  class Compiler
    class DefineBlockInstruction < BaseInstruction
      def initialize(arity:)
        @arity = arity
      end

      def has_body?
        true
      end

      attr_reader :arity

      def to_s
        'define_block'
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(expected_label: :define_block)
        fn = transform.temp('block')
        transform.with_new_scope(body) do |t|
          body = []
          body << "Value #{fn}(Env *env, Value self, Args args, Block *block) {"
          body << t.transform('return')
          body << '}'
          transform.top(body)
        end
        transform.push("(new Block(env, self, #{fn}, #{@arity}))")
      end

      def execute(vm)
        vm.push(Natalie::VM::Block.new(vm: vm))
        vm.skip_block_of_instructions(expected_label: :define_block)
        nil
      end

      def serialize(_)
        [
          instruction_number,
          arity,
        ].pack('Cw')
      end

      def self.deserialize(io, _)
        arity = io.read_ber_integer
        new(arity: arity)
      end
    end
  end
end
