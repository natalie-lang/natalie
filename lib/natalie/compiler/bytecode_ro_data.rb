module Natalie
  class Compiler
    class BytecodeRoData
      def initialize
        @buffer = ''.b
        @add_lookup = {}
      end

      def add(value)
        return @add_lookup[value] if @add_lookup.key?(value)

        size = value.bytesize
        position = @buffer.size
        @buffer << [value.bytesize].pack('w')
        @buffer << value.b
        @add_lookup[value.b] = position
        position
      end

      def bytesize = @buffer.bytesize
      def empty? = @buffer.empty?
      def to_s = @buffer
    end
  end
end
