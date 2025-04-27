require_relative './base_instruction'

module Natalie
  class Compiler
    class ArgsEmptyInstruction < BaseInstruction
      def initialize(include_keyword_hash:)
        @include_keyword_hash = include_keyword_hash
      end

      def to_s
        s = 'args_empty'
        s << ' (include_keyword_hash)' if @include_keyword_hash
        s
      end

      def generate(transform)
        transform.exec_and_push(:args_empty, "bool_object(args.size(#{@include_keyword_hash}) == 0)")
      end

      def execute(vm)
        count = vm.args.size
        count -= 1 if vm.kwargs&.any? && !@include_keyword_hash
        vm.push(count.zero?)
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
