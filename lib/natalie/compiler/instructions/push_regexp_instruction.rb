require_relative './base_instruction'

module Natalie
  class Compiler
    class PushRegexpInstruction < BaseInstruction
      include StringToCpp

      def initialize(regexp, encoding: nil)
        @regexp = regexp
        @encoding = encoding
      end

      def to_s
        str = "push_regexp #{@regexp.inspect}"
        str += " (#{@encoding.name})" if @encoding
        str
      end

      def generate(transform)
        transform.exec_and_push(:regexp, "Value(RegexpObject::literal(env, #{string_to_cpp(@regexp.source)}, #{@regexp.options}, #{encoding}))")
      end

      def execute(vm)
        regexp = @regexp.dup
        regexp = Regexp.compile(regexp.source.dup.force_encoding(@encoding), Regexp::FIXEDENCODING) if @encoding
        vm.push(regexp)
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

      private

      def encoding
        case @encoding
        when Encoding::EUC_JP
          'EncodingObject::get(Encoding::EUC_JP)'
        else
          'nullptr';
        end
      end
    end
  end
end
