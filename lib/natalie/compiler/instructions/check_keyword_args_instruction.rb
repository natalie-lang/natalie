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

      def generate(transform)
        transform.exec(
          'args.check_keyword_args(env, ' \
            "{ #{@required_keywords.map { |kw| transform.intern(kw) }.join(', ')} }, " \
            "{ #{@optional_keywords.map { |kw| transform.intern(kw) }.join(', ')} }, " \
            "Args::KeywordRestType::#{@keyword_rest.capitalize})",
        )
      end
    end
  end
end
