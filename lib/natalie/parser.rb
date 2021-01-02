if RUBY_ENGINE != 'natalie'
  require 'ruby_parser'
end

module Natalie
  class Parser
    class IncompleteExpression < StandardError; end

    def initialize(code_str, path)
      @code_str = code_str
      @path = path
    end

    def ast
      if RUBY_ENGINE == 'natalie'
        natalie_parse
      else
        ruby_parse
      end
    end

    private

    def natalie_parse
      ::Parser.parse(@code_str, @path)
    end

    def ruby_parse
      node = RubyParser.new.parse(@code_str, @path)
      if node.nil?
        s(:block)
      elsif node.first == :block
        node
      else
        s(:block, node)
      end
    rescue Racc::ParseError => e
      if e.message =~ /\$end/
        raise IncompleteExpression, e.message
      else
        raise
      end
    end
  end
end
