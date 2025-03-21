module Natalie
  class Compiler
    module ComptimeValues
      def comptime_array_of_strings(node, path: nil)
        raise_comptime_value_error('array', node, path) unless node.is_a?(::Prism::ArrayNode)

        node.elements.map { |item| comptime_string(item, path:) }
      end

      def comptime_string(node, path: nil)
        if node.is_a?(::Prism::InterpolatedStringNode) && node.parts.all?(::Prism::StringNode)
          unescaped = node.parts.each_with_object(+'') { |part, memo| memo << part.unescaped }
          node = node.parts.first.copy(unescaped:)
        end

        raise_comptime_value_error('string', node, path) unless node.type == :string_node

        node.unescaped
      end

      def comptime_symbol(node, path: nil)
        raise_comptime_value_error('symbol', node, path) unless node.type == :symbol_node
        node.unescaped.to_sym
      end

      def raise_comptime_value_error(expected, node, path)
        raise ArgumentError,
              "expected #{expected} at compile time, but got: #{node.type} " \
                "(#{path}##{node.location.start_line})"
      end
    end
  end
end
