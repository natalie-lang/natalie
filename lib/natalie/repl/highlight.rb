require_relative 'stringutils'
require_relative 'constants'

module Natalie
  class HighlightingRule
    def highlight(token, prev_token)
      token
    end

    def self.evaluate_all(rules, text)
      prev_token = nil
      with_each_token(text) do |token, _, _, is_last|
        next token if is_last
        prev_token = token
        current_prev_token = prev_token.clone
        maybe_highlighted = token
        rules.each do |rule|
          highlighted = rule.highlight(token, current_prev_token)
          unless highlighted == token
            maybe_highlighted = highlighted
            break
          end
        end
        maybe_highlighted
      end
    end
  end

  class RegexHighlightingRule < HighlightingRule
    def initialize(regex, out_pattern)
      @regex = regex
      @out_pattern = out_pattern
    end

    def highlight(token, prev_token)
      token.gsub(@regex, "#{@out_pattern}#{RESET_STYLE_ASCCI_CODE}")
    end
  end

  KEYWORD_HIGHLIGHT = RegexHighlightingRule.new(/(#{RUBY_KEYWORDS.join('|')})/, "\u001b[38;5;206m\\1")
  PASCAL_CASE_HIGLIGHT = RegexHighlightingRule.new(/([A-Z][A-Za-z0-9]*)/, "\u001b[38;5;70m\\1")
  CAMEL_CASE_HIGLIGHT = RegexHighlightingRule.new(/([a-z][A-Za-z0-9]*)/, "\u001b[38;5;220m\\1")
end
