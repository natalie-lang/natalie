require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushArgInstruction < BaseInstruction
      def initialize(index)
        @index = index
      end

      attr_reader :index

      def to_s
        "push_arg #{@index}"
      end

      def to_cpp(transform)
        "args[#{@index}]"
      end
    end
  end
end
