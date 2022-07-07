module Natalie
  class Compiler2
    class Args
      def initialize(pass, file:, line:)
        @pass = pass
        @file = file
        @line = line
      end

      def transform(exp)
        @from_side = :left
        @instructions = []
        _, *@args = exp
        while @args.any?
          arg = @from_side == :left ? @args.shift : @args.pop
          if transform_arg(arg) == :reverse
            @from_side = { left: :right, right: :left }.fetch(@from_side)
          end
        end
        clean_up
      end

      private

      def transform_arg(arg)
        if arg.is_a?(Sexp)
          case arg.sexp_type
          when :kwarg
            transform_keyword_arg(arg)
          when :lasgn
            clean_up_keyword_args
            transform_optional_arg(arg)
          when :masgn
            clean_up_keyword_args
            transform_destructured_arg(arg)
          else
            raise "I don't yet know how to compile #{arg.inspect}"
          end
        elsif arg.start_with?('**')
          transform_keyword_splat_arg(arg)
        elsif arg.start_with?('*')
          clean_up_keyword_args
          transform_splat_arg(arg)
        else
          clean_up_keyword_args
          transform_required_arg(arg)
        end
      end

      def remaining_required_args
        @args.select { |arg| !arg.is_a?(Sexp) && !arg.start_with?('*') }
      end

      def remaining_keyword_args
        @args.select { |arg| arg.is_a?(Sexp) && arg.sexp_type == :kwarg }
      end

      def kwsplat?
        @args.any? { |arg| arg.is_a?(Symbol) && arg.start_with?('**') }
      end

      def transform_required_arg(arg)
        shift_or_pop_next_arg
        @instructions << variable_set(arg)
      end

      def transform_optional_arg(arg)
        if remaining_required_args.any?
          # we cannot steal a value that might be needed to fulfill a required arg that follows
          # so put it back and work from the right side
          @args.unshift(arg)
          return :reverse
        elsif remaining_keyword_args.any? || kwsplat?
          # we cannot steal the keyword hash as if it is a regular arg,
          # so put it back and work from the right side
          @args.unshift(arg)
          return :reverse
        elsif @from_side == :right
          # optional args must be assigned from the left-to-right;
          # if we arrived here, it must be because we fulfilled all the required args on the right
          # and now we can start from the left again
          @args.push(arg)
          return :reverse
        end
        _, name, default_value = arg
        @instructions << @pass.transform_expression(default_value, used: true)
        shift_or_pop_next_arg_with_default
        @instructions << variable_set(name)
      end

      def transform_keyword_arg(arg)
        _, name, default = arg
        move_keyword_arg_hash_from_args_array_to_stack
        if default
          @instructions << @pass.transform_expression(default, used: true)
          @instructions << HashDeleteWithDefaultInstruction.new(name)
        else
          @instructions << HashDeleteInstruction.new(name, for_keyword_arg: true)
        end
        @instructions << variable_set(name)
      end

      def transform_destructured_arg(arg)
        @instructions << ArrayShiftInstruction.new
        @instructions << DupInstruction.new
        @instructions << ToArrayInstruction.new
        sub_processor = self.class.new(@pass, file: @file, line: @line)
        @instructions << sub_processor.transform(arg)
        @instructions << PopInstruction.new
      end

      def transform_splat_arg(arg)
        name = arg[1..-1]
        if name.empty?
          :noop
        else
          @instructions << variable_set(name)
          @instructions << VariableGetInstruction.new(name) # TODO: could eliminate this if the *splat is the last arg
        end
        :reverse
      end

      def transform_keyword_splat_arg(arg)
        name = arg[2..-1]
        move_keyword_arg_hash_from_args_array_to_stack
        if name.empty?
          :noop
        else
          @instructions << variable_set(name)
          @instructions << VariableGetInstruction.new(name) # TODO: could eliminate this if the **splat is the last arg
        end
        :reverse
      end

      def variable_set(name)
        name = name[1..] if name.start_with?('&')
        raise "bad var name: #{name.inspect}" unless name =~ /^[a-z_][a-z0-9_]*/
        VariableSetInstruction.new(name, local_only: true)
      end

      def shift_or_pop_next_arg
        if @from_side == :left
          @instructions << ArrayShiftInstruction.new
        else
          @instructions << ArrayPopInstruction.new
        end
      end

      def shift_or_pop_next_arg_with_default
        if @from_side == :left
          @instructions << ArrayShiftWithDefaultInstruction.new
        else
          @instructions << ArrayPopWithDefaultInstruction.new
        end
      end

      def move_keyword_arg_hash_from_args_array_to_stack
        return if @keyword_arg_hash_on_stack
        @instructions << ArrayPopKeywordArgsInstruction.new
        @keyword_arg_hash_on_stack = true
      end

      def clean_up_keyword_args
        @instructions << PopInstruction.new if @keyword_arg_hash_on_stack
        @keyword_arg_hash_on_stack = false
      end

      def clean_up
        clean_up_keyword_args
        @instructions << PopInstruction.new
      end
    end
  end
end
