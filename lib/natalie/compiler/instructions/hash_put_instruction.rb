require_relative './base_instruction'

module Natalie
  class Compiler
    class HashPutInstruction < BaseInstruction
      def to_s
        'hash_put'
      end

      def generate(transform)
        value = transform.pop
        key = transform.pop
        hash = transform.peek
        transform.exec("#{hash}.as_hash()->put(env, #{key}, #{value})")
      end

      def execute(vm)
        value = vm.pop
        key = vm.pop
        hash = vm.peek
        hash[key] = value
      end
    end
  end
end
