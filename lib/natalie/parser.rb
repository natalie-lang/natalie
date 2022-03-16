module Natalie
  class Parser
    class IncompleteExpression < StandardError
    end

    # enable dog-fooding our own parser by passing nat_parser: true
    def initialize(code_str, path, nat_parser: false)
      if RUBY_ENGINE != 'natalie'
        if nat_parser
          so_ext = RUBY_PLATFORM =~ /darwin/ ? 'bundle' : 'so'
          begin
            $LOAD_PATH << File.expand_path('../../build/natalie_parser/lib', __dir__)
            $LOAD_PATH << File.expand_path('../../build/natalie_parser/ext', __dir__)
            require 'natalie_parser'
          rescue LoadError
            puts 'Error: You must build natalie_parser.so by running: rake parser_c_ext'
            exit 1
          end
          #require_relative '../../lib/sexp_stub_for_use_by_c_ext'
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
