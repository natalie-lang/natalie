require_relative './base_instruction'

module Natalie
  class Compiler2
    class HashDeleteInstruction < BaseInstruction
      def initialize(name, for_keyword_arg: false)
        @name = name.to_sym
        @for_keyword_arg = for_keyword_arg
      end

      def to_s
        s = "hash_delete #{@name}"
        s << ' (for keyword arg)' if @for_keyword_arg
        s
      end

      def generate(transform)
        ary = transform.peek
        if @for_keyword_arg
          transform.exec_and_push(@name, "#{ary}->as_hash()->remove_keyword_arg(env, #{@name.to_s.inspect}_s)")
        else
          transform.exec_and_push(@name, "#{ary}->as_hash()->remove(env, #{@name.to_s.inspect}_s)")
        end
      end

      def execute(vm)
        hash = vm.peek
        vm.push(hash.delete(@name))
      end
    end
  end
end
