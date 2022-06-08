require_relative './instructions'

module Natalie
  class Compiler2
    class BasePass
      def inspect
        "<#{self.class.name}:0x#{object_id.to_s(16)}>"
      end
    end
  end
end
