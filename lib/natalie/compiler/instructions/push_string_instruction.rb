require_relative './base_instruction'
require_relative '../string_to_cpp'

module Natalie
  class Compiler
    class PushStringInstruction < BaseInstruction
      include StringToCpp

      def initialize(string, bytesize: string.bytesize, encoding: Encoding::UTF_8, frozen: false)
        super()
        @string = string
        @bytesize = bytesize
        @encoding = encoding
        @frozen = frozen
      end

      def to_s
        "push_string #{@string.inspect}, #{@bytesize}, #{@encoding.name}#{@frozen ? ', frozen' : ''}"
      end

      def generate(transform)
        enum = @encoding.name.tr('-', '_').upcase
        encoding_object = "EncodingObject::get(Encoding::#{enum})"
        name = if @string.empty?
                 transform.exec_and_push(:string, "Value(new StringObject(#{encoding_object}))")
               else
                 transform.exec_and_push(
                   :string,
                   "Value(new StringObject(#{string_to_cpp(@string)}, (size_t)#{@bytesize}, #{encoding_object}))"
                 )
               end
        transform.exec("#{name}->freeze()") if @frozen
      end

      def execute(vm)
        string = @string.dup.force_encoding(@encoding)
        string.freeze if @frozen
        vm.push(string)
      end

      def serialize(rodata)
        position = rodata.add(@string)
        encoding = rodata.add(@encoding.to_s)

        [
          instruction_number,
          position,
          encoding,
          @frozen ? 1 : 0,
        ].pack("CwwC")
      end

      def self.deserialize(io, rodata)
        string_position = io.read_ber_integer
        encoding_position = io.read_ber_integer
        frozen = io.getbool
        string = rodata.get(string_position, convert: :to_s)
        string = string.dup unless frozen
        encoding = rodata.get(encoding_position, convert: :to_encoding)
        new(string, bytesize: string.bytesize, encoding: encoding, frozen: frozen)
      end
    end
  end
end
