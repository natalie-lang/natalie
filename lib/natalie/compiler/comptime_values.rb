module Natalie
  class Compiler
    module ComptimeValues
      def comptime_array_of_strings(exp)
        unless exp.sexp_type == :array
          raise "Expected array at compile time, but got: #{exp.inspect}"
        end
        exp[1..].map do |item|
          comptime_string(item)
        end
      end

      def comptime_string(exp)
        unless exp.sexp_type == :str
          raise "Expected string at compile time, but got: #{exp.inspect}"
        end
        exp.last
      end

      def comptime_symbol(exp)
        unless exp.sexp_type == :lit && exp.last.is_a?(Symbol)
          raise "Expected symbol at compile time, but got: #{exp.inspect}"
        end
        exp.last
      end
    end
  end
end
