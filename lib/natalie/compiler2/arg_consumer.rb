module Natalie
  class Compiler2
    class ArgConsumer
      def initialize
        @from_side = :left
      end

      attr_reader :from_side

      def consume(exp)
        _, *args = exp
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
