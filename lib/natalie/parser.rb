module Natalie
  class Parser
    class IncompleteExpression < StandardError
    end

    # enable dog-fooding our own parser by passing nat_parser: true
    def initialize(code_str, path, nat_parser: false)
      if RUBY_ENGINE != 'natalie'
        if nat_parser
          begin
            require_relative '../../build/parser_c_ext'
          rescue LoadError
            puts 'Error: You must build parser_c_ext.so by running: rake build/parser_c_ext.so'
            exit 1
          end
        else
          require 'ruby_parser'
        end
      end

      @code_str = code_str
      @path = path
      @nat_parser = nat_parser
    end

    def ast
      RUBY_ENGINE == 'natalie' || @nat_parser ? natalie_parse : ruby_parse
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
