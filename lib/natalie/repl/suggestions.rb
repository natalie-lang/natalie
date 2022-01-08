require_relative 'stringutils'
require_relative 'constants'

module Natalie
  class SuggestionProvider
    def initialize(vars)
      @vars = vars
    end

    def suggest(text, index)
      normalized_index = index - 1
      with_each_token(text) do |token, left, right, is_last|
        next token unless (is_last && left <= normalized_index && normalized_index <= right && token.length > 1)
        maybe_suggestion = (RUBY_KEYWORDS + @vars.keys).each.select { |keyword| keyword.start_with?(token) }.first
        next token_with_suggestion(token, maybe_suggestion) if maybe_suggestion
        token
      end
    end

    def token_with_suggestion(token, suggestion)
      "#{token}\u001b[38;5;241m#{suggestion[(token.length)..]}#{RESET_STYLE_ASCCI_CODE}"
    end
  end
end
