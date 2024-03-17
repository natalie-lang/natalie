require_relative './instruction_manager'
require_relative './instructions'
require_relative '../vm'

module Natalie
  class Compiler
    class BytecodeLoader
      INSTRUCTIONS = Natalie::Compiler::INSTRUCTIONS

      def initialize(io)
        @io = IO.new(io)
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

      def load_instructions
        instructions = []
        loop do
          num = @io.getbyte
          break if num.nil?

          instruction_class = INSTRUCTIONS[num]
          instructions << instruction_class.deserialize(@io)
        end
        instructions
      end
    end
  end
end
