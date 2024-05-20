module Natalie
  class Compiler
    module Bytecode
      class RoData
        CONVERSIONS = {
          to_encoding: ->(str) { Encoding.find(str) },
          to_s: ->(str) { str },
          to_sym: ->(str) { str.to_sym },
        }.freeze

        def initialize
          @buffer = ''.b
          @add_lookup = {}
          @get_lookup = {}
        end

        def self.load(str)
          rodata = new
          rodata.send(:buffer=, str)
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

        def get(position, convert: nil)
          return @get_lookup[[convert, position]] if convert && @get_lookup.key?([convert, position])
          size = @buffer[position..].unpack1('w')
          result = @buffer[position + 1, size]
          if convert
            result = CONVERSIONS.fetch(convert).call(result)
            @get_lookup[[convert, position]] = result.freeze
          end
          result
        end

        def bytesize = @buffer.bytesize
        def empty? = @buffer.empty?
        def to_s = @buffer

        private

        attr_writer :buffer
      end
    end
  end
end
