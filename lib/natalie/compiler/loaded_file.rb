module Natalie
  class Compiler
    class LoadedFile
      def initialize(path:, ast: nil, instructions: nil, encoding: Encoding::UTF_8)
        @path = path
        @ast = ast
        @instructions = instructions
        @encoding = encoding
      end

      attr_reader :path, :ast, :encoding

      attr_accessor :instructions

      def relative_path
        if File.absolute_path?(@path) && @path.start_with?(Dir.pwd)
          @path[(Dir.pwd.size + 1)..-1]
        else
          @path
        end
      end
    end
  end
end
