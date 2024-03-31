module Natalie
  class Compiler
    module Bytecode
      class Sections
        SECTIONS = {
          1 => :CODE,
          2 => :RODATA,
        }.freeze

        attr_reader :bytecode_offset

        def initialize(header:, rodata:, bytecode:)
          @header_size = header.bytesize
          if rodata.nil? || rodata.empty?
            @bytecode_offset = @header_size + 6
          else
            @rodata_offset = @header_size + 6
            @bytecode_offset = @rodata_offset + 4 + rodata.bytesize
          end
        end

        def self.load(io)
          result = allocate
          io.read(1).unpack1('C').times do
            type, offset = io.read(5).unpack('CN')
            case SECTIONS[type]
            when :CODE
              result.send(:bytecode_offset=, offset)
            when :RODATA
              result.send(:rodata_offset=, offset)
            else
              allowed = SECTIONS.map { |k, v| "#{k} (#{v})" }.join(', ')
              raise "Invalid section identifier, expected any of #{allowed}, got #{id}"
            end
          end
          result
        end

        def rodata?
          @rodata_offset && !@rodata_offset.zero?
        end

        def rodata_offset
          raise 'No RODATA section found' unless rodata?
          @rodata_offset
        end

        def bytesize
          result = 1 # 8 bits Number of sections
          result += 5 unless @rodata_size.zero? # 8 bits identifier + 32 bits size
          result += 5 # code, 8 bits identifer + 32 bits size
          result
        end

        # Format: number of sections (8 bits)
        #         for every section: section id (8 bits), section offset (32 bits)
        # Don't use variable width size here: we need to be predictable on where to put the sections
        def to_s
          number_of_sections = 1
          rodata = ''.b
          if rodata?
            number_of_sections += 1
            rodata = [SECTIONS.key(:RODATA), @rodata_offset].pack('CN')
          end
          result = [number_of_sections].pack('C')
          result << rodata
          result << [SECTIONS.key(:CODE), @bytecode_offset].pack('CN')
          result
        end

        private

        attr_writer :bytecode_offset, :rodata_offset
      end
    end
  end
end
