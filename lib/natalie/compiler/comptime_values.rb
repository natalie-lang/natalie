module Natalie
  class Compiler
    module ComptimeValues
      def comptime_array_of_strings(node, path: nil)
        unless node.is_a?(::Prism::ArrayNode)
          raise_comptime_value_error('array', node, path)
        end

        node.elements.map { |item| comptime_string(item, path:) }
      end

      def comptime_string(node, path: nil)
        if node.is_a?(::Prism::InterpolatedStringNode) && node.parts.all?(::Prism::StringNode)
          string_node = node.parts.first
          node.parts[1..].each do |next_node|
            string_node.content << next_node.content
            string_node.unescaped << next_node.unescaped
          end
          node = string_node
        end

        unless node.type == :string_node
          raise_comptime_value_error('string', node, path)
        end

        node.unescaped
      end

      def comptime_symbol(node, path: nil)
        unless node.type == :symbol_node
          raise_comptime_value_error('symbol', node, path)
        end
        node.unescaped.to_sym
      end

      def raise_comptime_value_error(expected, node, path)
        raise ArgumentError, "expected #{expected} at compile time, but got: #{node.type} " \
                             "(#{path}##{node.location.start_line})"
      end
    end
  end
end
