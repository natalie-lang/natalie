require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushArgcInstruction < BaseInstruction
      def initialize(count)
        @count = count
      end

      attr_reader :count

      def to_s
        "push_argc #{@count}"
      end

      def to_cpp(transform)
        @count
      end
    end
  end
end
