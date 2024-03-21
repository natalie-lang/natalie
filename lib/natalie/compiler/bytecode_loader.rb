require_relative './instruction_manager'
require_relative './instructions'
require_relative '../vm'

module Natalie
  class Compiler
    class BytecodeLoader
      INSTRUCTIONS = Natalie::Compiler::INSTRUCTIONS

      def initialize(io)
        @io = IO.new(io)
        validate_signature
        @instructions = load_instructions
      end

      attr_reader :instructions

      class IO
        def initialize(io)
          @io = io
        end

        def getbool
          getbyte == 1
        end

        def getbyte
          @io.getbyte
        end

        def read(size)
          @io.read(size)
        end

        def read_ber_integer
          result = 0
          loop do
            byte = getbyte
            result += (byte & 0x7f)
            if (byte & 0x80) == 0
              break
            end
            result <<= 7
          end
          result
        end
      end

      private

      def validate_signature
        header, major_version, minor_version = @io.read(6).unpack('a4C2')
        raise 'Invalid header, this is probably not a Natalie bytecode file' if header != 'NatX'
        # For now, mark the version as 0.0. The bytecode format is unfinished and there is no backwards compatiblity
        # guarantee.
        raise "Invalid version, expected 0.0, got #{major_version}.#{minor_version}" if major_version != 0 || minor_version != 0
      end

      def load_instructions
        instructions = []
        loop do
          num = @io.getbyte
          break if num.nil?

          instruction_class = INSTRUCTIONS.fetch(num)
          instructions << instruction_class.deserialize(@io)
        end
        instructions
      end
    end
  end
end
