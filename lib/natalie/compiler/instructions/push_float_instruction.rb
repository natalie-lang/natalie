require_relative './base_instruction'

module Natalie
  class Compiler
    class PushFloatInstruction < BaseInstruction
      def initialize(float)
        @float = float
      end

      attr_reader :float

      def to_s
        "push_float #{@float}"
      end

      def generate(transform)
        if @float.infinite?
          if @float.positive?
            transform.push("Value(FloatObject::positive_infinity(env))")
          else
            transform.push("Value(FloatObject::negative_infinity(env))")
          end
        else
          transform.push("Value(new FloatObject(#{@float}))")
        end
      end

      def execute(vm)
        vm.push(@float)
      end

      def serialize(_)
        [
          instruction_number,
          @float,
        ].pack('CG')
      end

      def self.deserialize(io, _)
        float = io.read(8).unpack1('G')
        new(float)
      end
    end
  end
end
