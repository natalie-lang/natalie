require_relative './base_instruction'

module Natalie
  class Compiler
    class CheckKeywordArgsInstruction < BaseInstruction
      def initialize(required_keywords:, optional_keywords:, keyword_rest:)
        @required_keywords = required_keywords
        @optional_keywords = optional_keywords
        @keyword_rest = keyword_rest
        unless %i[none present forbidden].include?(@keyword_rest)
          raise 'keyword_rest must be :none, :present, or :forbidden'
        end
      end

      def to_s
        'check_keyword_args ' \
          "required: #{@required_keywords.join(', ')}; " \
          "optional: #{@optional_keywords.join(', ')}; " \
          "keyword_rest: #{@keyword_rest}"
      end

      def generate_llvm(transform)
        required = transform.keyword_names(@required_keywords)
        optional = transform.keyword_names(@optional_keywords)
        rest = { none: 0, present: 1, forbidden: 2 }.fetch(@keyword_rest)
        transform.emit "call void @nat_llvm_check_keywords(ptr %env, ptr %args, i64 #{@required_keywords.size}, ptr #{required}, i64 #{@optional_keywords.size}, ptr #{optional}, i32 #{rest})"
      end
    end
  end
end
