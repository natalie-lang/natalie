require_relative './arg_consumer'

module Natalie
  class Compiler
    class MultipleAssignment
      def initialize(pass, file:, line:)
        @pass = pass
        @file = file
        @line = line
        @consumer = ArgConsumer.new
      end

      def transform(exp)
        @instructions = []
        @consumer.consume(exp) do |arg|
          transform_arg(arg)
        end
        clean_up
      end

      private

      def transform_arg(arg)
        if arg.is_a?(Sexp)
          case arg.sexp_type
          when :cdecl
            _, name = arg
            transform_constant(name)
          when :iasgn
            _, name = arg
            transform_instance_variable(name)
          when :gasgn
            _, name = arg
            transform_global_variable(name)
          when :lasgn
            _, name = arg
            transform_variable(name)
          when :masgn
            _, names_array = arg
            transform_destructured_arg(names_array)
          when :splat
            _, name = arg
            transform_splat_arg(name)
          when :attrasgn, :call
            _, receiver, message, *args = arg
            transform_attr_assign_arg(receiver, message, args)
          else
            raise "I don't yet know how to compile #{arg.inspect} #{arg.file}##{arg.line}"
          end
        else
          raise "I don't yet know how to compile #{arg.inspect}"
        end
      end

      def transform_attr_assign_arg(receiver, message, args)
        shift_or_pop_next_arg
        @instructions << @pass.transform_expression(receiver, used: true)
        if args.any?
          args.each { |arg| @instructions << @pass.transform_expression(arg, used: true) }
        end
        @instructions << MoveRelInstruction.new(args.size + 1) # move value after args
        @instructions << PushArgcInstruction.new(args.size + 1)
        @instructions << SendInstruction.new(
          message,
          receiver_is_self: false,
          with_block: false,
          file: @file,
          line: @line,
        )
        @instructions << PopInstruction.new
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
        return :reverse if arg.nil? # nameless splat

        case arg.sexp_type
        when :gasgn
          _, name = arg
          @instructions << GlobalVariableSetInstruction.new(name)
          @instructions << GlobalVariableGetInstruction.new(name)
        when :iasgn
          _, name = arg
          @instructions << InstanceVariableSetInstruction.new(name)
          @instructions << InstanceVariableGetInstruction.new(name)
        when :lasgn
          _, name = arg
          @instructions << variable_set(name)
          @instructions << VariableGetInstruction.new(name)
        when :attrasgn
          _, receiver, message = arg
          @instructions << @pass.transform_expression(receiver, used: true)
          @instructions << SwapInstruction.new
          @instructions << PushArgcInstruction.new(1)
          @instructions << SendInstruction.new(
            message,
            receiver_is_self: false,
            with_block: false,
            file: @file,
            line: @line,
          )
        else
          raise "I don't yet know how to compile splat arg #{arg.inspect}"
        end

        :reverse
      end

      def transform_constant(name)
        shift_or_pop_next_arg
        name, prep_instruction = constant_name(name)
        @instructions << prep_instruction
        @instructions << ConstSetInstruction.new(name)
      end

      def transform_global_variable(name)
        shift_or_pop_next_arg
        @instructions << GlobalVariableSetInstruction.new(name)
      end

      def transform_instance_variable(name)
        shift_or_pop_next_arg
        @instructions << InstanceVariableSetInstruction.new(name)
      end

      def transform_variable(name)
        shift_or_pop_next_arg
        @instructions << variable_set(name)
      end

      def shift_or_pop_next_arg
        if @consumer.from_side == :left
          @instructions << ArrayShiftInstruction.new
        else
          @instructions << ArrayPopInstruction.new
        end
      end

      def shift_or_pop_next_arg_with_default
        if @consumer.from_side == :left
          @instructions << ArrayShiftWithDefaultInstruction.new
        else
          @instructions << ArrayPopWithDefaultInstruction.new
        end
      end

      # returns a pair of [name, prep_instruction]
      # prep_instruction being the instruction(s) needed to get the owner of the constant
      def constant_name(name)
        prepper = ConstPrepper.new(name, pass: self)
        [prepper.name, prepper.namespace]
      end

      def variable_set(name)
        raise "bad var name: #{name.inspect}" unless name =~ /^[a-z_][a-z0-9_]*/
        VariableSetInstruction.new(name, local_only: false)
      end

      def clean_up
        @instructions << PopInstruction.new
      end
    end
  end
end
