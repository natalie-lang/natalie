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
        case arg.type
        when :call_node
          transform_attr_assign_arg(arg.receiver, arg.name, arg.arguments)
        when :constant_target_node
          transform_constant(arg.name)
        when :global_variable_target_node
          transform_global_variable(arg.name)
        when :instance_variable_target_node
          transform_instance_variable(arg.name)
        when :local_variable_target_node
          transform_variable(arg.name)
        when :multi_target_node
          transform_destructured_arg(arg)
        when :splat_node
          transform_splat_arg(arg.expression)
        else
          raise "I don't yet know how to compile #{arg.inspect} #{arg.location.path}##{arg.location.start_line}"
        end
      end

      def transform_attr_assign_arg(receiver, message, args_node)
        args = args_node&.arguments || []
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
        if arg.targets.size == 1 && arg.targets.first.is_a?(::Prism::SplatNode)
          # Prism always wraps a SplatNode in a MultiTargetNode?
          return transform_splat_arg(arg.targets.first.expression)
        end

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
        when :call_node
          @instructions << @pass.transform_expression(arg.receiver, used: true)
          @instructions << SwapInstruction.new
          @instructions << PushArgcInstruction.new(1)
          @instructions << SendInstruction.new(
            arg.name,
            receiver_is_self: false,
            with_block: false,
            file: @file,
            line: @line,
          )
        when :global_variable_target_node
          @instructions << GlobalVariableSetInstruction.new(arg.name)
          @instructions << GlobalVariableGetInstruction.new(arg.name)
        when :instance_variable_target_node
          @instructions << InstanceVariableSetInstruction.new(arg.name)
          @instructions << InstanceVariableGetInstruction.new(arg.name)
        when :local_variable_target_node
          @instructions << variable_set(arg.name)
          @instructions << VariableGetInstruction.new(arg.name)
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
