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
        start_ip = vm.ip
        vm.skip_block_of_instructions(expected_label: :define_block)
        parent_scope = vm.scope
        captured_self = vm.self
        captured_block = vm.block
        block_lambda =
          lambda do |*args|
            vm.with_self(captured_self) do
              scope = { vars: {}, parent: parent_scope }
              vm.push_call(name: nil, return_ip: vm.ip, args: args, scope: scope, block: captured_block)
              vm.ip = start_ip
              begin
                vm.run
              ensure
                vm.ip = vm.pop_call[:return_ip]
              end
              vm.pop # result must be returned from proc
            end
          end
        vm.push(block_lambda)
      end

      def serialize
        [
          instruction_number,
          arity,
        ].pack("Cw")
      end
    end
  end
end
