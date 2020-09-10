require 'ruby_parser'

module Natalie
  class Parser
    class IncompleteExpression < StandardError; end

    def initialize(code_str, path)
      @code_str = code_str
      @path = path
    end

    def ast
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
