require_relative './base_instruction'

module Natalie
  class Compiler
    class HashMergeInstruction < BaseInstruction
      def to_s
        'hash_merge'
      end

      def generate(transform)
        hash2 = transform.pop
        hash = transform.peek
        transform.exec("#{hash}.as_hash()->merge_in_place(env, Args({ #{hash2} }), nullptr)")
      end

      def execute(vm)
        hash2 = vm.pop
        hash = vm.peek
        hash.merge!(hash2)
      end
    end
  end
end
