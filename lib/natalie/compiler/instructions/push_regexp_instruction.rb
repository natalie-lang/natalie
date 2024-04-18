require_relative './base_instruction'

module Natalie
  class Compiler
    class PushRegexpInstruction < BaseInstruction
      include StringToCpp

      def initialize(regexp)
        @regexp = regexp
      end

      def to_s
        "push_regexp #{@regexp.inspect}"
      end

      def generate(transform)
        transform.exec_and_push(:regexp, "Value(RegexpObject::literal(env, #{string_to_cpp(@regexp.source)}, #{@regexp.options}))")
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
