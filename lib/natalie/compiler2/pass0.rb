require_relative './base_pass'
require_relative 'optimizations/ast_based'

module Natalie
  class Compiler2
    # AST based optimization pass
    class Pass0 < BasePass
      def initialize(context, ast)
        @context = context
        @ast = ast
      end

      def transform()
        raise 'unexpected AST input' unless @ast.sexp_type == :block
        for optimization in AST_BASED_OPTIMIZATIONS
          # FIXME: We maybe we should reenable the flag we have an enabling statement after a disabling one
          if (
               @context[:options][:optimization_level] >= optimization[:optimization_level] ||
                 optimization[:enable_flags].any? { |flag| @context[:options][:optimizations]&.include?(flag) }
             ) && !optimization[:disable_flags].any? { |flag| @context[:options][:optimizations]&.include?(flag) }
            @ast = optimization[:class].new(@context).go(@ast)
          end
        end

        return @ast
      end
    end
  end
end
