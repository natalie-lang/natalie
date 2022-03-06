module Natalie
  class VM
    class MainObject
      def inspect
        'main'
      end

      def define_method(name, &block)
        self.class.define_method(name, &block)
      end
    end
  end
end
