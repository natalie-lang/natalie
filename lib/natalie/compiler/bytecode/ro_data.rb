module Natalie
  class Compiler
    module Bytecode
      class RoData
        def initialize
          @buffer = ''.b
          @add_lookup = {}
        end

        def self.load(str)
          rodata = new
          rodata.instance_variable_set(:@buffer, str)
          rodata
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

        def get(position)
          size = @buffer[position..].unpack1('w')
          @buffer[position + 1, size]
        end

        def bytesize = @buffer.bytesize
        def empty? = @buffer.empty?
        def to_s = @buffer
      end
    end
  end
end
