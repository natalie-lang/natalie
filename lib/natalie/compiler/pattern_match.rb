module Natalie
  class Compiler
    class PatternMatch
      def initialize(pass, required:)
        @pass = pass
        @required = required
      end

      def transform(node)
        raise "unexpected node: #{node.inspect}" unless node.is_a?(Prism::MatchRequiredNode)

        @instructions = []
        @instructions << @pass.transform_expression(node.value, used: true)

        transform_pattern(node.pattern)

        # Pattern matching assoc is a void value expression,
        # so we need to pop whatever we started with and push nil.
        @instructions << PopInstruction.new
        @instructions << PushNilInstruction.new

        @instructions
      end

      private

      def transform_pattern(pattern)
        case pattern
        when Prism::ArrayPatternNode
          transform_array_pattern(pattern)
        when Prism::HashPatternNode
          transform_hash_pattern(pattern)
        when Prism::LocalVariableTargetNode
          transform_local_variable_target_node(pattern)
        when Prism::IntegerNode
          transform_literal_node(pattern)
        else
          raise "I don't yet know how to compile the pattern: #{pattern.inspect}"
        end
      end

      def transform_array_pattern(pattern)
        @instructions << ToArrayInstruction.new
        @instructions << PatternArraySizeCheckInstruction.new(length: pattern.requireds.size)
        pattern.requireds.each do |node|
          @instructions << ArrayShiftInstruction.new
          transform_pattern(node)
        end
        if pattern.rest || pattern.posts.any?
          raise "I don't yet know how to compile rest or posts for the array pattern: #{pattern.inspect}"
        end
      end

      def transform_hash_pattern(pattern)
        @instructions << ToHashInstruction.new
        pattern.elements.each do |node|
          if node.key.is_a?(Prism::SymbolNode)
            @instructions << HashDeleteInstruction.new(node.key.unescaped)
            transform_pattern(node.value)
          else
            raise "I don't yet know how to compile the hash key pattern: #{node.value.inspect}"
          end
        end
      end

      def transform_literal_node(node)
        @instructions << @pass.transform_expression(node, used: true)
        @instructions << PatternEqualCheckInstruction.new
      end

      def transform_local_variable_target_node(node)
        @instructions << VariableSetInstruction.new(node.name, local_only: true)
      end
    end
  end
end
