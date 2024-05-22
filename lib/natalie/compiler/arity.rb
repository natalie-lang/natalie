module Natalie
  class Compiler
    class Arity
      def initialize(args, is_proc:)
        args = args.parameters if args.is_a?(Prism::BlockParametersNode)

        @node = args

        case args
        when nil
          @args = []
        when ::Prism::ParametersNode
          @args = (
            args.requireds +
            [args.rest] +
            args.optionals +
            args.posts +
            args.keywords +
            [args.keyword_rest]
          ).compact
        when ::Prism::NumberedParametersNode
          @args = args.maximum.times.map do |i|
            Prism::RequiredParameterNode.new(nil, nil, :"_#{i + 1}", args.location)
          end
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
          arg.is_a?(::Prism::RestParameterNode) ||
            arg.is_a?(::Prism::KeywordRestParameterNode)
        end
      end

      def required_count
        args.count do |arg|
          arg.is_a?(::Prism::RequiredParameterNode) ||
            arg.is_a?(::Prism::MultiTargetNode)
        end
      end

      def optional_count
        splat_count + optional_named_count
      end

      def optional_named_count
        return 0 if @is_proc

        args.count do |arg|
          arg.is_a?(::Prism::OptionalParameterNode)
        end
      end

      def required_keyword_count
        args.count do |arg|
          arg.is_a?(::Prism::RequiredKeywordParameterNode)
        end
      end

      def optional_keyword_count
        return 0 if @is_proc

        args.count do |arg|
          arg.is_a?(::Prism::OptionalKeywordParameterNode)
        end
      end
    end
  end
end
