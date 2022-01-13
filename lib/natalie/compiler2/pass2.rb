require_relative './base_pass'

module Natalie
  class Compiler2
    class Pass2 < BasePass
      def initialize(instructions)
        @instructions = instructions
      end

      def transform
        # TODO: do more stuff here?
        @instructions
      end
    end
  end
end
