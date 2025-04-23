require_relative './base_instruction'

module Natalie
  class Compiler
    class ArgsShiftInstruction < BaseInstruction
      def initialize(include_keyword_hash:)
        @include_keyword_hash = include_keyword_hash
      end

      def to_s
        s = 'args_shift'
        s << ' (include_keyword_hash)' if @include_keyword_hash
        s
      end

      def generate(transform)
        transform.exec_and_push(:first_arg, "args.shift(env, #{@include_keyword_hash ? 'true' : 'false'})")
      end

      def execute(vm)
        vm.push(vm.args.shift)
      end

      def serialize(_)
        [instruction_number, @include_keyword_hash ? 1 : 0].pack('CC')
      end

      def self.deserialize(io, _)
        include_keyword_hash = io.getbool
        new(include_keyword_hash:)
      end
    end
  end
end
