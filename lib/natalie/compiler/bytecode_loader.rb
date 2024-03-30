require_relative './bytecode'
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
        sections = load_sections
        # sections[1] is the offset for code
        @io.read(4) # Ignore section size for now
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
        if major_version != Bytecode::MAJOR_VERSION || minor_version != Bytecode::MINOR_VERSION
          raise "Invalid version, expected #{Bytecode::MAJOR_VERSION}.#{Bytecode::MINOR_VERSION}, got #{major_version}.#{minor_version}"
        end
      end

      def load_sections
        num_sections = @io.getbyte
        num_sections.times.each_with_object({}) do |_, sections|
          id = @io.getbyte
          type = Bytecode::SECTIONS[id]
          if type.nil?
            allowed = Bytecode::SECTIONS.map { |k, v| "#{k} (#{v})" }.join(', ')
            raise "Invalid section identifier, expected any of #{allowed}, got #{id}"
          end
          offset = @io.read(4).unpack1('N')
          sections[id] = offset
        end
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
