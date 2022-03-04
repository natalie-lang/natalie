require_relative './base_instruction'

module Natalie
  class Compiler2
    # use:
    # push(new_name)
    # push(old_name)
    # alias
    class AliasInstruction < BaseInstruction
      def to_s
        'alias'
      end

      def generate(transform)
        old_name = transform.pop
        new_name = transform.pop
        transform.exec("self->alias(env, #{new_name}, #{old_name})")
      end

      def execute(vm)
        old_name = vm.pop
        new_name = vm.pop
        vm.self.instance_eval do
          alias_method new_name, old_name
        end
      end
    end
  end
end
