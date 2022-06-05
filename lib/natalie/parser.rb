module Natalie
  class Parser
    class IncompleteExpression < StandardError
    end

    def initialize(code_str, path)
      so_ext = RUBY_PLATFORM =~ /darwin/ ? 'bundle' : 'so'

      if RUBY_ENGINE != 'natalie'
        begin
          $LOAD_PATH << File.expand_path('../../build/natalie_parser/lib', __dir__)
          $LOAD_PATH << File.expand_path('../../build/natalie_parser/ext', __dir__)
          require 'natalie_parser'
        rescue LoadError
          puts 'Error: You must build natalie_parser.so by running: rake parser_c_ext'
          exit 1
        end
      end

      @code_str = code_str
      @path = path
    end

    def ast
      result = ::NatalieParser.parse(@code_str, @path)
      if result.is_a?(Sexp) && result.sexp_type == :block
        result
      else
        result.new(:block, result)
      end
    end
  end
end
