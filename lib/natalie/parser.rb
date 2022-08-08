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
        rescue LoadError => e
          if e.message =~ /incompatible library version/
            # NOTE: It's actually NatalieParser that was built against a different Ruby version,
            # but saying that might be more confusing.
            puts 'Error: Natalie was built with a different version of Ruby.'
            puts "Please switch your current Ruby version or rebuild Natalie with version #{RUBY_VERSION}."
          else
            puts 'Error: You must build natalie_parser.so by running: rake parser_c_ext'
          end
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
