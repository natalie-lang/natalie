module Natalie
  class Compiler
    class Arity
      def initialize(args, is_proc:)
        if args.nil? || args == 0
          @args = []
        elsif args.is_a?(::Prism::Node) && %i[parameters_node block_parameters_node].include?(args.type)
          # NOTE: More info about sorted parameters: https://github.com/ruby/prism/issues/1436
          @args = args.child_nodes.compact.sort_by { |n| n.location.start_offset }.dup
          @args.pop if @args.last.is_a?(Symbol) && @args.last.start_with?('&')
          @args.pop if @args.last&.type == :block_parameter_node
        else
          raise "expected args node, but got: #{args.inspect}"
        end
        @is_proc = is_proc
      end

      attr_reader :args

      def arity
        num = required_count
        opt = optional_count
        if required_keyword_count > 0
          num += 1
        elsif optional_keyword_count > 0
          opt += 1
        end
        num = -num - 1 if opt > 0
        num
      end

      private

      def splat_count
        args.count do |arg|
          if arg.is_a?(::Prism::Node)
            arg.type == :rest_parameter_node ||
              arg.type == :keyword_rest_parameter_node
          else
            arg.is_a?(Symbol) && arg.start_with?('*')
          end
        end
      end

      def required_count
        args.count do |arg|
          arg.type == :required_parameter_node ||
            arg.type == :required_destructured_parameter_node
        end
      end

      def optional_count
        splat_count + optional_named_count
      end

      def optional_named_count
        return 0 if @is_proc
        args.count do |arg|
          arg.type == :optional_parameter_node
        end
      end

      def required_keyword_count
        args.count do |arg|
          arg.type == :keyword_parameter_node && !arg.value
        end
      end

      def optional_keyword_count
        return 0 if @is_proc
        args.count do |arg|
          arg.type == :keyword_parameter_node && arg.value
        end
      end
    end
  end
end
