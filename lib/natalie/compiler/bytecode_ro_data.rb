module Natalie
  class Compiler
    class BytecodeRoData
      def initialize
        @buffer = ''.b
      end

      def add(value)
        size = value.bytesize
        position = @buffer.size
        @buffer << [value.bytesize].pack('w')
        @buffer << value.b
        position
      end
    end
  end
end
