module Natalie
  class Compiler
    class Args
      def initialize(node:, pass:, check_args:, local_only: true)
        @node = node
        @pass = pass
        @check_args = check_args
        @local_only = local_only
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
          @node.requireds.each { |arg| transform_required_arg(arg) }
          @node.posts.reverse_each { |arg| transform_required_post_arg(arg) }
          @node.optionals.each { |arg| transform_optional_arg(arg) }
          transform_rest_arg(@node.rest) if @node.rest
          @node.keywords.each { |arg| transform_keyword_arg(arg) }
          transform_keyword_rest_arg(@node.keyword_rest) if @node.keyword_rest
        when Prism::LocalVariableTargetNode
          transform_required_arg(@node)
        when Prism::ItParametersNode
          transform_it_arg(@node)
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
          raise "unhandled node: #{@node.inspect}"
        end
        clean_up
        @instructions
      end

      private

      def simple?
        return true if @node.nil?
        return false unless @node.is_a?(Prism::ParametersNode)
        return false if @node.requireds.any? { |arg| arg.type != :required_parameter_node }
        if @node.optionals.any? || @node.posts.any? || @node.rest || @node.keywords.any? || @node.keyword_rest
          return false
        end
        return false if @node.requireds.count { |a| a.type == :required_parameter_node && a.name == :_ } > 1

        true
      end

      def transform_simply
        args = @node&.requireds || []

        if @check_args
          @instructions << CheckArgsInstruction.new(positional: args.size, has_keywords: false, required_keywords: [])
        end

        args.each_with_index do |arg, index|
          @instructions << PushArgInstruction.new(index, nil_default: false)
          @instructions << VariableSetInstruction.new(arg.name, local_only: @local_only)
        end

        @instructions
      end

      def do_setup
        min_count = minimum_arg_count
        max_count = maximum_arg_count

        if @check_args
          argc = min_count == max_count ? min_count : min_count..max_count
          @instructions << CheckArgsInstruction.new(
            positional: argc,
            has_keywords: any_keyword_args?,
            required_keywords:,
          )
        end

        if any_keyword_args?
          @instructions << CheckKeywordArgsInstruction.new(
            required_keywords:,
            optional_keywords:,
            keyword_rest: keyword_rest_type,
          )
        end

        @instructions << PopKeywordArgsInstruction.new if any_keyword_args?

        @instructions << PushArgsInstruction.new(for_block: false, min_count:, max_count:, spread: false)
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

      def transform_it_arg(arg)
        @instructions << VariableDeclareInstruction.new(:it)
        @instructions << ArrayShiftInstruction.new
        @instructions << variable_set(:it)
      end

      def transform_numbered_arg(arg)
        arg.maximum.times do |i|
          @instructions << ArrayShiftInstruction.new
          @instructions << variable_set(:"_#{i + 1}")
        end
      end

      def transform_multi_target_arg(node)
        node.lefts.each { |arg| transform_required_arg(arg) }
        node.rights.reverse_each { |arg| transform_required_post_arg(arg) }
        transform_rest_arg(node.rest) if node.rest
      end

      def transform_optional_arg(arg)
        name = arg.name
        default_value = arg.value

        if default_value&.type == :local_variable_read_node && default_value.name == name
          default_value = Prism.nil_node(location: default_value.location)
        end

        @instructions << ArrayIsEmptyInstruction.new
        @instructions << IfInstruction.new
        @instructions << @pass.transform_expression(default_value, used: true)
        @instructions << ElseInstruction.new(:if)
        @instructions << ArrayShiftInstruction.new
        @instructions << EndInstruction.new(:if)
        @instructions << variable_set(name)
      end

      def transform_keyword_arg(arg)
        swap_keyword_arg_hash_with_args_array
        case arg
        when Prism::RequiredKeywordParameterNode
          @instructions << HashDeleteInstruction.new(arg.name)
          @instructions << variable_set(arg.name)
        when Prism::OptionalKeywordParameterNode
          @instructions << HashHasKeyInstruction.new(arg.name)
          @instructions << IfInstruction.new
          @instructions << HashDeleteInstruction.new(arg.name)
          @instructions << ElseInstruction.new(:if)
          @instructions << @pass.transform_expression(arg.value, used: true)
          @instructions << EndInstruction.new(:if)
          @instructions << variable_set(arg.name)
        else
          raise "unhandled node: #{arg.inspect}"
        end
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
          receiver_is_self: arg.receiver.is_a?(Prism::SelfNode),
          with_block: false,
          has_keyword_hash: false,
          file: @pass.file.path,
          line: arg.location.start_line,
        )
        @instructions << EndInstruction.new(:if) if arg.safe_navigation?
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
          receiver_is_self: arg.receiver.is_a?(Prism::SelfNode),
          with_block: false,
          has_keyword_hash: false,
          file: @pass.file.path,
          line: arg.location.start_line,
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
          unless arg.expression.type == :required_parameter_node
            raise "I don't know how to splat #{arg.expression.inspect}"
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
        when Prism::NoKeywordsParameterNode, Prism::ForwardingParameterNode
          :noop
        when Prism::KeywordRestParameterNode
          if arg.name
            @instructions << variable_set(arg.name)
            @instructions << VariableGetInstruction.new(arg.name)
          else
            @instructions << AnonymousKeywordSplatSetInstruction.new
            @instructions << AnonymousKeywordSplatGetInstruction.new
          end
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
        @instructions << PopInstruction.new if @keyword_arg_hash_ready
        @keyword_arg_hash_ready = false
      end

      def clean_up
        clean_up_keyword_args
        @instructions << PopInstruction.new
      end

      def args_to_array
        @args_to_array ||=
          case @node
          when nil
            []
          when Prism::ParametersNode
            (@node.requireds + [@node.rest] + @node.optionals + @node.posts).compact
          when Prism::NumberedParametersNode
            @node.maximum.times.map { |i| Prism::RequiredParameterNode.new(nil, nil, @node.location, 0, :"_#{i + 1}") }
          when Prism::ItParametersNode
            [Prism::RequiredParameterNode.new(nil, nil, @node.location, 0, :it)]
          else
            [@node]
          end
      end

      def minimum_arg_count
        args_to_array.count { |arg| arg.type == :required_parameter_node || arg.type == :multi_target_node }
      end

      def maximum_arg_count
        return nil if @node.is_a?(Prism::ParametersNode) && @node.rest.is_a?(Prism::RestParameterNode)
        return nil if @node.is_a?(Prism::ParametersNode) && @node.keyword_rest.is_a?(Prism::ForwardingParameterNode)

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

        @node.keywords.filter_map { |arg| arg.name if arg.type == :required_keyword_parameter_node }
      end

      def optional_keywords
        return [] unless @node.is_a?(Prism::ParametersNode)

        @node.keywords.filter_map { |arg| arg.name if arg.type == :optional_keyword_parameter_node }
      end

      def keyword_rest_type
        return :none unless @node.is_a?(Prism::ParametersNode)

        case @node.keyword_rest&.type
        when :no_keywords_parameter_node
          :forbidden
        when :keyword_rest_parameter_node, :forwarding_parameter_node
          :present
        when nil
          :none
        else
          raise "unhandled keyword rest type: #{@node.keyword_rest.type}"
        end
      end

      def has_keyword_rest?
        return false unless @node.is_a?(Prism::ParametersNode)

        !!@node.keyword_rest
      end

      def any_keyword_args?
        return false unless @node.is_a?(Prism::ParametersNode)

        @node.keywords.any? || has_keyword_rest?
      end
    end
  end
end
