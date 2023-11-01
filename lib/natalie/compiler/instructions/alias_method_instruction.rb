require_relative './base_instruction'

module Natalie
  class Compiler
    # use:
    # push(new_name)
    # push(old_name)
    # alias
    class AliasMethodInstruction < BaseInstruction
      def to_s
        'alias_method'
      end

      def generate(transform)
        old_name = transform.pop
        new_name = transform.pop
        transform.exec("self->method_alias(env, #{new_name}, #{old_name})")
      end

      def execute(vm)
        old_name = vm.pop
        new_name = vm.pop
        if vm.self.respond_to?(:alias_method)
          vm.self.alias_method(new_name, old_name)
        else
          vm.self.class.alias_method(new_name, old_name)
        end
      end
    end
  end
end
