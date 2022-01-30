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
            transform_destructured_arg(arg)
          when :lasgn
            transform_optional_arg(arg)
          else
            raise "I don't yet know how to compile #{arg.inspect}"
          end
        elsif arg.start_with?('*')
          transform_splat_arg(arg)
        else
          transform_required_arg(arg)
        end
      end

      def remaining_required_args
        @args.reject { |arg| arg.is_a?(Sexp) && arg.sexp_type == :lasgn }
      end

      def transform_destructured_arg(arg)
        @instructions << ArrayShiftInstruction.new
        sub_processor = Args.new(@pass)
        @instructions << sub_processor.transform(arg)
      end

      def transform_optional_arg(arg)
        if remaining_required_args.any?
          # we cannot steal a value that might be needed to fulfill a required arg that follows
          # so put it back and work from the right side
          @args.unshift(arg)
          return :reverse
        end
        _, name, default_value = arg
        @instructions << @pass.transform_expression(default_value, used: true)
        shift_or_pop_next_arg_with_default
        @instructions << VariableSetInstruction.new(name, local_only: true)
      end

      def transform_splat_arg(arg)
        name = arg[1..-1]
        @instructions << VariableSetInstruction.new(name, local_only: true)
        @instructions << VariableGetInstruction.new(name) # TODO: could eliminate this if the *splat is the last arg
        :reverse
      end

      def transform_required_arg(arg)
        shift_or_pop_next_arg
        @instructions << VariableSetInstruction.new(arg, local_only: true)
      end

      def shift_or_pop_next_arg
        @instructions << (@from_side == :left ? ArrayShiftInstruction.new : ArrayPopInstruction.new)
      end

      def shift_or_pop_next_arg_with_default
        if @from_side == :left
          @instructions << ArrayShiftWithDefaultInstruction.new
        else
          @instructions << ArrayPopWithDefaultInstruction.new
        end
      end
    end
  end
end
