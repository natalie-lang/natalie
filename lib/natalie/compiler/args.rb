module Natalie
  class Compiler
    class Args
      def initialize(node:, pass:, check_args:, local_only: true, for_block: false)
        @node = node
        @pass = pass
        @check_args = check_args
        @local_only = local_only
        @for_block = for_block
        @underscore_arg_set = false
      end

      def transform
        @instructions = []
        return transform_simply if simple?

        do_setup
        case @node
        when Prism::MultiTargetNode
          @instructions << ArrayShiftInstruction.new
          @instructions << ToArrayInstruction.new
          transform_multi_target_arg(@node)
          @instructions << PopInstruction.new
        when Prism::ParametersNode
          @node.requireds.each do |arg|
            transform_required_arg(arg)
          end
          @node.posts.reverse_each do |arg|
            transform_required_post_arg(arg)
          end
          @node.optionals.each do |arg|
            transform_optional_arg(arg)
          end
          if @node.rest
            transform_rest_arg(@node.rest)
          end
          @node.keywords.each do |arg|
            transform_keyword_arg(arg)
          end
          if @node.keyword_rest
            transform_keyword_rest_arg(@node.keyword_rest)
          end
        when Prism::LocalVariableTargetNode
          transform_required_arg(@node)
        when Prism::ItParametersNode
          @instructions << ArrayShiftInstruction.new
          @instructions << variable_set(:it)
        when ::Prism::NumberedParametersNode
          transform_numbered_arg(@node)
        when ::Prism::InstanceVariableTargetNode
          transform_instance_variable_arg(@node)
        when ::Prism::ClassVariableTargetNode
          transform_class_variable_arg(@node)
        when ::Prism::GlobalVariableTargetNode
          transform_global_variable_arg(@node)
        when ::Prism::ConstantTargetNode
          transform_constant_arg(@node)
        when ::Prism::CallTargetNode
          transform_call_arg(@node)
        when ::Prism::IndexTargetNode
          transform_index_arg(@node)
        else
          raise "unhandled node: #{node.inspect}"
        end
        clean_up
        @instructions
      end

      private

      def simple?
        return true if @node.nil?
        return false unless @node.is_a?(Prism::ParametersNode)
        return false if @node.requireds.any? { |arg| arg.type != :required_parameter_node }
        return false if @node.optionals.any? || @node.posts.any? || @node.rest || @node.keywords.any? || @node.keyword_rest
        return false if @node.requireds.count { |a| a.type == :required_parameter_node && a.name == :_ } > 1
        return false if @for_block && @node.requireds.size > 1

        true
      end

      def transform_simply
        args = @node&.requireds || []

        if @check_args
          @instructions << CheckArgsInstruction.new(positional: args.size, keywords: [])
        end

        args.each_with_index do |arg, index|
          @instructions << PushArgInstruction.new(index, nil_default: @for_block)
          @instructions << VariableSetInstruction.new(arg.name, local_only: @local_only)
        end

        @instructions
      end

      def do_setup
        min_count = minimum_arg_count
        max_count = maximum_arg_count

        @instructions << PopKeywordArgsInstruction.new if any_keyword_args?

        if @check_args
          argc = min_count == max_count ? min_count : min_count..max_count
          @instructions << CheckArgsInstruction.new(positional: argc, keywords: required_keywords)
        end

        if required_keywords.any?
          @instructions << CheckRequiredKeywordsInstruction.new(required_keywords)
        end

        spread = @for_block && args_to_array.size > 1
        @instructions << PushArgsInstruction.new(for_block: @for_block, min_count:, max_count:, spread:)
      end

      def transform_required_arg(arg)
        case arg
        when Prism::MultiTargetNode
          @instructions << ArrayShiftInstruction.new
          @instructions << ToArrayInstruction.new
          transform_multi_target_arg(arg)
          @instructions << PopInstruction.new
        when Prism::RequiredParameterNode, Prism::LocalVariableTargetNode
          @instructions << ArrayShiftInstruction.new
          @instructions << variable_set(arg.name)
        else
          raise "unhandled node: #{arg.inspect}"
        end
      end

      def transform_required_post_arg(arg)
        case arg
        when Prism::MultiTargetNode
          @instructions << ArrayPopInstruction.new
          @instructions << ToArrayInstruction.new
          transform_multi_target_arg(arg)
          @instructions << PopInstruction.new
        when Prism::RequiredParameterNode, Prism::LocalVariableTargetNode
          @instructions << ArrayPopInstruction.new
          @instructions << variable_set(arg.name)
        else
          raise "unhandled node: #{arg.inspect}"
        end
      end

      def transform_numbered_arg(arg)
        arg.maximum.times do |i|
          @instructions << ArrayShiftInstruction.new
          @instructions << variable_set(:"_#{i + 1}")
        end
      end

      def transform_multi_target_arg(node)
        node.lefts.each do |arg|
          transform_required_arg(arg)
        end
        node.rights.reverse_each do |arg|
          transform_required_post_arg(arg)
        end
        if node.rest
          transform_rest_arg(node.rest)
        end
      end

      def transform_optional_arg(arg)
        name = arg.name
        default_value = arg.value

        if default_value&.type == :local_variable_read_node && default_value.name == name
          raise SyntaxError, "circular argument reference - #{name}"
        end

        @instructions << @pass.transform_expression(default_value, used: true)
        @instructions << ArrayShiftWithDefaultInstruction.new
        @instructions << variable_set(name)
      end

      def transform_keyword_arg(arg)
        swap_keyword_arg_hash_with_args_array
        case arg
        when Prism::RequiredKeywordParameterNode
          @instructions << HashDeleteInstruction.new(arg.name)
          @instructions << variable_set(arg.name)
        when Prism::OptionalKeywordParameterNode
          @instructions << @pass.transform_expression(arg.value, used: true)
          @instructions << HashDeleteWithDefaultInstruction.new(arg.name)
          @instructions << variable_set(arg.name)
        else
          raise "unhandled node: #{arg.inspect}"
        end
      end

      def transform_required_keyword_arg(arg)
        swap_keyword_arg_hash_with_args_array
        @instructions << HashDeleteInstruction.new(arg.name)
        @instructions << variable_set(arg.name)
      end

      def transform_optional_keyword_arg(arg)
        swap_keyword_arg_hash_with_args_array
        @instructions << @pass.transform_expression(arg.value, used: true)
        @instructions << HashDeleteWithDefaultInstruction.new(arg.name)
        @instructions << variable_set(arg.name)
      end

      def transform_instance_variable_arg(arg)
        @instructions << ArrayShiftInstruction.new
        @instructions << InstanceVariableSetInstruction.new(arg.name)
      end

      def transform_class_variable_arg(arg)
        @instructions << ArrayShiftInstruction.new
        @instructions << ClassVariableSetInstruction.new(arg.name)
      end

      def transform_global_variable_arg(arg)
        @instructions << ArrayShiftInstruction.new
        @instructions << GlobalVariableSetInstruction.new(arg.name)
      end

      def transform_constant_arg(arg)
        @instructions << ArrayShiftInstruction.new
        @instructions << PushSelfInstruction.new
        @instructions << ConstSetInstruction.new(arg.name)
      end

      def transform_call_arg(arg)
        @instructions << ArrayShiftInstruction.new
        @instructions.concat(@pass.transform_expression(arg.receiver, used: true))
        if arg.safe_navigation?
          @instructions << DupInstruction.new
          @instructions << IsNilInstruction.new
          @instructions << IfInstruction.new
          @instructions << PopInstruction.new
          @instructions << ElseInstruction.new(:if)
        end
        @instructions << SwapInstruction.new
        @instructions << PushArgcInstruction.new(1)
        @instructions << SendInstruction.new(
          arg.name,
          args_array_on_stack: false,
          receiver_is_self:    arg.receiver.is_a?(Prism::SelfNode),
          with_block:          false,
          has_keyword_hash:    false,
          file:                @pass.file.path,
          line:                arg.location.start_line,
        )
        if arg.safe_navigation?
          @instructions << EndInstruction.new(:if)
        end
      end

      def transform_index_arg(arg)
        @instructions << ArrayShiftInstruction.new
        @instructions.concat(@pass.transform_expression(arg.receiver, used: true))
        @instructions << SwapInstruction.new
        arg.arguments.arguments.each do |argument|
          @instructions.concat(@pass.transform_expression(argument, used: true))
          @instructions << SwapInstruction.new
        end
        @instructions << PushArgcInstruction.new(arg.arguments.arguments.size + 1)
        @instructions << SendInstruction.new(
          :[]=,
          args_array_on_stack: false,
          receiver_is_self:    arg.receiver.is_a?(Prism::SelfNode),
          with_block:          false,
          has_keyword_hash:    false,
          file:                @pass.file.path,
          line:                arg.location.start_line,
        )
      end

      def transform_rest_arg(arg)
        case arg
        when Prism::SplatNode
          transform_splat_arg(arg)
        when Prism::RestParameterNode
          if (name = arg.name)
            @instructions << variable_set(name)
            @instructions << VariableGetInstruction.new(name)
          else
            @instructions << AnonymousSplatSetInstruction.new
            @instructions << AnonymousSplatGetInstruction.new
          end
        when Prism::ImplicitRestNode
          :noop
        else
          raise "unhandled node: #{arg.inspect}"
        end
      end

      def transform_splat_arg(arg)
        if arg.expression
          if @for_block
            unless %i[local_variable_target_node required_parameter_node].include?(arg.expression.type)
              raise "I don't know how to splat #{arg.expression.inspect}"
            end
          else
            unless arg.expression.type == :required_parameter_node
              raise "I don't know how to splat #{arg.expression.inspect}"
            end
          end

          name = arg.expression.name
        end
        if name
          @instructions << variable_set(name)
          @instructions << VariableGetInstruction.new(name)
        end
      end

      def transform_keyword_rest_arg(arg)
        swap_keyword_arg_hash_with_args_array
        case arg
        when Prism::NoKeywordsParameterNode
          :noop
        when Prism::KeywordRestParameterNode
          if arg.name
            @instructions << variable_set(arg.name)
            @instructions << VariableGetInstruction.new(arg.name)
          else
            @instructions << AnonymousKeywordSplatSetInstruction.new
            @instructions << AnonymousKeywordSplatGetInstruction.new
          end
          @has_keyword_rest = true
        else
          raise "unhandled node: #{arg.inspect}"
        end
      end

      def variable_set(name)
        raise "bad var name: #{name.inspect}" unless name =~ /^(?:[[:alpha:]]|_)[[:alnum:]]*/

        if name == :_
          if @underscore_arg_set
            # don't keep setting the same _ argument
            return PopInstruction.new
          else
            @underscore_arg_set = true
          end
        end

        VariableSetInstruction.new(name, local_only: @local_only)
      end

      def swap_keyword_arg_hash_with_args_array
        return if @keyword_arg_hash_ready

        @instructions << SwapInstruction.new
        @keyword_arg_hash_ready = true
      end

      def clean_up_keyword_args
        if @keyword_arg_hash_ready
          @instructions << CheckExtraKeywordsInstruction.new unless @has_keyword_rest
          @instructions << PopInstruction.new
        end
        @keyword_arg_hash_ready = false
      end

      def clean_up
        clean_up_keyword_args
        @instructions << PopInstruction.new
      end

      def args_to_array
        @flatten_args ||= case @node
        when nil
          []
        when Prism::ParametersNode
          (
            @node.requireds +
            [@node.rest] +
            @node.optionals +
            @node.posts +
            @node.keywords +
            [@node.keyword_rest]
          ).compact
        when Prism::NumberedParametersNode
          @node.maximum.times.map do |i|
            Prism::RequiredParameterNode.new(nil, nil, @node.location, 0, :"_#{i + 1}")
          end
        else
          [@node]
        end
      end

      def minimum_arg_count
        args_to_array.count do |arg|
          arg.type == :required_parameter_node || arg.type == :multi_target_node
        end
      end

      def maximum_arg_count
        return nil if @node.is_a?(Prism::ParametersNode) && @node.rest.is_a?(Prism::RestParameterNode)

        args_to_array.count do |arg|
          %i[
            call_target_node
            class_variable_target_node
            constant_target_node
            global_variable_target_node
            index_target_node
            instance_variable_target_node
            local_variable_target_node
            multi_target_node
            optional_parameter_node
            required_parameter_node
          ].include?(arg.type)
        end
      end

      def required_keywords
        return [] unless @node.is_a?(Prism::ParametersNode)

        @node.keywords.filter_map do |arg|
          arg.name if arg.type == :required_keyword_parameter_node
        end
      end

      def any_keyword_args?
        return false unless @node.is_a?(Prism::ParametersNode)

        @node.keywords.any? || !!@node.keyword_rest
      end
    end
  end
end
