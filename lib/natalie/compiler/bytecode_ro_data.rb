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

      def bytesize = @buffer.bytesize
      def empty? = @buffer.empty?
      def to_s = @buffer
    end
  end
end
