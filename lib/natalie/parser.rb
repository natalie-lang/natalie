module Natalie
  class Parser
    class IncompleteExpression < StandardError
    end

    def initialize(code_str, path)
      so_ext = RUBY_PLATFORM =~ /darwin/ ? 'bundle' : 'so'

      begin
        $LOAD_PATH << File.expand_path('../../build/natalie_parser/lib', __dir__)
        $LOAD_PATH << File.expand_path('../../build/natalie_parser/ext', __dir__)
        require 'natalie_parser'
      rescue LoadError
        puts 'Error: You must build natalie_parser.so by running: rake parser_c_ext'
        exit 1
      end

      @code_str = code_str
      @path = path
    end

    def ast
      ::NatalieParser.parse(@code_str, @path)
    end
  end
end
