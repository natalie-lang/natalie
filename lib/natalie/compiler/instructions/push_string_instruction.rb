require_relative './base_instruction'
require_relative '../string_to_cpp'

module Natalie
  class Compiler
    class PushStringInstruction < BaseInstruction
      include StringToCpp

      def initialize(string, bytesize: string.bytesize)
        @string = string
        @bytesize = bytesize
      end

      def to_s
        "push_string #{@string.inspect}, #{@bytesize}"
      end

      def generate(transform)
        if @string.empty?
          transform.exec_and_push(:string, "Value(new StringObject)")
        else
          transform.exec_and_push(:string, "Value(new StringObject(#{string_to_cpp(@string)}, (size_t)#{@bytesize}))")
        end
      end

      def execute(vm)
        vm.push(@string.dup)
      end

      def serialize
        [
          instruction_number,
          @bytesize,
          @string,
        ].pack("Cwa#{@bytesize}")
      end

      def self.deserialize(io)
        size = io.read_ber_integer
        string = io.read(size)
        new(string, bytesize: size)
      end
    end
  end
end
