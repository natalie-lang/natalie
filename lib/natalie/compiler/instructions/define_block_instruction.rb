require_relative './base_instruction'

module Natalie
  class Compiler
    class DefineBlockInstruction < BaseInstruction
      def initialize(arity:, for_lambda: false, has_return: false, parameters: [])
        @arity = arity
        @for_lambda = for_lambda
        @has_return = has_return
        @parameters = parameters
      end

      def has_body?
        true
      end

      def for_lambda? = !!@for_lambda
      def has_return? = !!@has_return

      attr_reader :arity, :parameters
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
        params_ref = @parameters.empty? ? 'nullptr' : transform.intern_param_table(@parameters)
        transform.push(
          "Block::create(*env, self, #{fn}, #{@arity}, #{has_return?}, Block::BlockType::Proc, #{params_ref})",
        )
      end

      def execute(vm)
        vm.push(Natalie::VM::Block.new(vm: vm))
        vm.skip_block_of_instructions(expected_label: :define_block)
        nil
      end

      def serialize(rodata)
        header = [instruction_number, arity, for_lambda? ? 1 : 0, has_return? ? 1 : 0].pack('CwCC')
        header + serialize_parameters(rodata, @parameters)
      end

      def self.deserialize(io, rodata)
        arity = io.read_ber_integer
        for_lambda = io.getbool
        has_return = io.getbool
        parameters = deserialize_parameters(io, rodata)
        new(arity:, for_lambda:, has_return:, parameters:)
      end
    end
  end
end
