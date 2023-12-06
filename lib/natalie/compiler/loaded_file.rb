module Natalie
  class Compiler
    class LoadedFile
      def initialize(path:, ast: nil, instructions: nil)
        @path = path
        @ast = ast
        @instructions = instructions
      end

      attr_accessor :ast,
                    :instructions
    end
  end
end
