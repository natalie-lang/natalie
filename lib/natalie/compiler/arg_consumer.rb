module Natalie
  class Compiler
    class ArgConsumer
      def initialize
        @from_side = :left
      end

      attr_reader :from_side

      def consume(exp)
        raise 'do not pass an Array to ArgConsumer' if exp.instance_of?(Array)
        args = exp.targets
        while args.any?
          if @from_side == :left
            arg = args.shift
          else
            arg = args.pop
          end
          if yield(arg) == :reverse
            reverse
          end
        end
      end

      private

      def reverse
        @from_side = { left: :right, right: :left }.fetch(@from_side)
      end
    end
  end
end
