require_relative './base_instruction'

module Natalie
  class Compiler
    class PopKeywordArgsInstruction < BaseInstruction
      def to_s
        'pop_keyword_args'
      end

      def generate(transform)
        transform.exec_and_push(
          :keyword_args_hash,
          'args.has_keyword_hash() ? args.pop_keyword_hash() : HashObject::create()',
        )
      end
    end
  end
end
