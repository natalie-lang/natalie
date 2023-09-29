module Natalie
  class Compiler
    module ComptimeValues
      def comptime_array_of_strings(node)
        unless node.sexp_type == :array
          raise_comptime_value_error('array', node)
        end
        node[1..].map do |item|
          comptime_string(item)
        end
      end

      def comptime_string(node)
        unless node.sexp_type == :str
          raise_comptime_value_error('string', node)
        end
        node.last
      end

      def comptime_symbol(node)
        unless node.sexp_type == :lit && node.last.is_a?(Symbol)
          raise_comptime_value_error('symbol', node)
        end
        node.last
      end

      def raise_comptime_value_error(expected, node)
          raise ArgumentError, "expected #{expected} at compile time, but got: #{node.inspect} (#{node.file}##{node.line})"
      end
    end
  end
end
