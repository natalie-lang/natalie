module Natalie
  class VM
    class MainObject
      def inspect
        'main'
      end

      def define_method(name, &block)
        self.class.define_method(name, &block)
      end

      def const_set(name, value)
        Object.const_set(name, value)
      end
    end
  end
end
