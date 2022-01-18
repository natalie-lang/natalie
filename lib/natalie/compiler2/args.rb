module Natalie
  class Compiler2
    class Args
      def initialize(pass)
        @pass = pass
      end

      def transform(exp)
        @from_side = :left
        @instructions = []
        _, *@args = exp
        while @args.any?
          arg = @from_side == :left ? @args.shift : @args.pop
          if transform_arg(arg) == :reverse
            break if @args.empty?
            arg = @args.pop
            @from_side = :right
            transform_arg(arg)
          end
        end
        @instructions << PopInstruction.new
      end

      private

      def transform_arg(arg)
        if arg.is_a?(Sexp)
          case arg.sexp_type
          when :masgn
            # destructuring an array
            @instructions << ArrayShiftInstruction.new
            sub_processor = Args.new(@pass)
            @instructions << sub_processor.transform(arg)
          else
            raise "I don't yet know how to compile #{arg.inspect}"
          end
        elsif arg.start_with?('*')
          # splat arg
          name = arg[1..-1]
          @instructions << VariableSetInstruction.new(name)
          @instructions << VariableGetInstruction.new(name) # TODO: could eliminate this if the *splat is the last arg
          :reverse
        else
          @instructions << (@from_side == :left ? ArrayShiftInstruction.new : ArrayPopInstruction.new)
          @instructions << VariableSetInstruction.new(arg)
        end
      end
    end
  end
end
