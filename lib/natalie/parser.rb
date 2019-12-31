require 'ruby_parser'

module Natalie
  class Parser
    def initialize(code_str)
      @code_str = code_str.strip + "\n"
    end

    def ast
      node = RubyParser.new.parse(@code_str)
      if node.nil?
        s(:block)
      elsif node.first == :block
        node
      else
        s(:block, node)
      end
    end
  end
end
