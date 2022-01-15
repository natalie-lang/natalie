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

      def generate(transform)
        body = transform.fetch_block_of_instructions(expected_label: :define_method)
        fn = transform.temp("defn_#{@name}")
        transform.with_new_scope(body) do |t|
          body = []
          body << "Value #{fn}(Env *env, Value self, size_t argc, Value *args, Block *block) {"
          body << t.transform('return')
          body << '}'
          transform.top(body)
        end
        transform.exec("self->define_method(env, #{@name.to_s.inspect}_s, #{fn}, #{@arity})")
        transform.push("#{@name.to_s.inspect}_s")
      end

      def execute(vm)
        start_ip = vm.ip
        vm.skip_block_of_instructions(expected_label: :define_method)
        vm
          .self
          .define_method(@name) do |*args|
            vm.push_call(return_ip: vm.ip, args: args)
            vm.ip = start_ip
            vm.run
            vm.ip = vm.pop_call[:return_ip]
            vm.pop # result must be returned to SendInstruction
          end
        case vm.method_visibility
        when :private
          vm.self.send(:private, @name)
        when :protected
          vm.self.send(:protected, @name)
        end
        vm.push(@name)
      end
    end
  end
end
