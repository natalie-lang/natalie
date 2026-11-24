require_relative './base_instruction'

module Natalie
  class Compiler
    class PopKeywordArgsInstruction < BaseInstruction
      def to_s
        'pop_keyword_args'
      end

      def generate_llvm(transform)
        transform.push_call('call i64 @nat_llvm_pop_keywords(ptr %args)')
      end
    end
  end
end
