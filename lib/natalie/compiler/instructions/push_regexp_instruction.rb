require_relative './base_instruction'

module Natalie
  class Compiler
    class PushRegexpInstruction < BaseInstruction
      include StringToCpp

      def initialize(regexp, euc_jp: false)
        @regexp = regexp
        @euc_jp = euc_jp
      end

      def to_s
        str = "push_regexp #{@regexp.inspect}"
        str += ' (euc-jp)' if @euc_jp
        str
      end

      def generate(transform)
        encoding = @euc_jp ? 'EncodingObject::get(Encoding::EUC_JP)' : 'nullptr';
        transform.exec_and_push(:regexp, "Value(RegexpObject::literal(env, #{string_to_cpp(@regexp.source)}, #{@regexp.options}, #{encoding}))")
      end

      def execute(vm)
        vm.push(@regexp.dup)
      end

      def serialize(rodata)
        position = rodata.add(@regexp.source)

        [
          instruction_number,
          position,
          @regexp.options,
        ].pack('Cww')
      end

      def self.deserialize(io, rodata)
        position = io.read_ber_integer
        regexp = rodata.get(position)
        options = io.read_ber_integer
        new(Regexp.new(regexp, options))
      end
    end
  end
end
