module Natalie
  class Compiler
    class LoadedFile
      def initialize(path:, ast: nil, instructions: nil, encoding: Encoding::UTF_8)
        @path = path
        @ast = ast
        @instructions = instructions
        @encoding = encoding
      end

      attr_accessor :ast,
                    :instructions,
                    :encoding
    end
  end
end
