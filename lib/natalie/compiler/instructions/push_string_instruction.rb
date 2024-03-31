require_relative './base_instruction'
require_relative '../string_to_cpp'

module Natalie
  class Compiler
    class PushStringInstruction < BaseInstruction
      include StringToCpp

      def initialize(string, bytesize: string.bytesize, encoding: Encoding::UTF_8)
        super()
        @string = string
        @bytesize = bytesize
        @encoding = encoding
      end

      def to_s
        "push_string #{@string.inspect}, #{@bytesize}, #{@encoding.name}"
      end

      def generate(transform)
        enum = @encoding.name.tr('-', '_').upcase
        encoding_object = "EncodingObject::get(Encoding::#{enum})"
        if @string.empty?
          transform.exec_and_push(:string, "Value(new StringObject(#{encoding_object}))")
        else
          transform.exec_and_push(
            :string,
            "Value(new StringObject(#{string_to_cpp(@string)}, (size_t)#{@bytesize}, #{encoding_object}))"
          )
        end
      end

      def execute(vm)
        vm.push(@string.dup.force_encoding(@encoding))
      end

      def serialize(rodata)
        position = rodata.add(@string)

        [
          instruction_number,
          position,
          Encoding.list.index(@encoding),
        ].pack("CwC")
      end

      def self.deserialize(io, rodata)
        position = io.read_ber_integer
        string = rodata.get(position)
        encoding = Encoding.list.at(io.getbyte)
        new(string, bytesize: string.bytesize, encoding: encoding)
      end
    end
  end
end
