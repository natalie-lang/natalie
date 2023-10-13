module Natalie
  class Compiler
    module ComptimeValues
      def comptime_array_of_strings(node)
        unless node.is_a?(::Prism::ArrayNode)
          raise_comptime_value_error('array', node)
        end

        node.elements.map { |item| comptime_string(item) }
      end

      def comptime_string(node)
        unless node.sexp_type == :str
          raise_comptime_value_error('string', node)
        end
        node.last
      end

      def comptime_symbol(node)
        unless node.sexp_type == :symbol_node
          raise_comptime_value_error('symbol', node)
        end
        node.unescaped.to_sym
      end

      def raise_comptime_value_error(expected, node)
        raise ArgumentError, "expected #{expected} at compile time, but got: #{node.inspect} (#{node.file}##{node.line})"
      end
    end
  end
end
