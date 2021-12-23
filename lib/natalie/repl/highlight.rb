module Natalie
  class HighlightingRule 
    def highlight(token, prev_token)
      token
    end

    def self.evaluate_all(rules, text) 
      l = 0
      copy = ""
      prev_token = nil
      text.length.times do |r|
        c = text[r]
        is_last = r == (text.length - 1)
        if /\s/.match?(c) || is_last
          token = text[if is_last then l..r else l...r end]

          maybe_highlighted_token = token
          rules.each do |rule|
            highlighted = rule.highlight(token, prev_token)
            unless highlighted == token
              maybe_highlighted_token = highlighted
              break
            end
          end
          
          if is_last
            copy.concat(maybe_highlighted_token)
          else
            copy.concat(maybe_highlighted_token, c)
          end

          prev_token = token
          l = r + 1
        end
      end
      copy
    end
  end
  
  class RegexHighlightingRule < HighlightingRule
    def initialize(regex, out_pattern)
      @regex = regex
      @out_pattern = out_pattern
    end

    def highlight(token, prev_token)
      token.gsub(@regex, "#{@out_pattern}\u001b[0m")
    end
  end

  RUBY_KEYWORDS = %w(__ENCODING__ __LINE__ __FILE__ BEGIN END alias and begin break case class def
    defined? do else elsif end ensure false for if in module next nil not or redo rescue retry return 
    self super then true undef unless until when while yield)

  KEYWORD_HIGHLIGHT = RegexHighlightingRule.new(/(#{RUBY_KEYWORDS.join('|')})/, "\u001b[38;5;206m\\1")
  PASCAL_CASE_HIGLIGHT = RegexHighlightingRule.new(/([A-Z][A-Za-z0-9]*)/, "\u001b[38;5;70m\\1")
  CAMEL_CASE_HIGLIGHT = RegexHighlightingRule.new(/([a-z][A-Za-z0-9]*)/, "\u001b[38;5;220m\\1")

end
