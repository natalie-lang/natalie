require_relative './base_pass'

module Natalie
  class Compiler2
    class Pass1 < BasePass
      def initialize(ast)
        @ast = ast
      end

      def transform
        raise 'unexpected AST input' unless @ast.sexp_type == :block
        transform_block(@ast, used: false).flatten
      end

      private

      def transform_array_of_expressions(array_of_expressions, used:)
        instructions = []
        array_of_expressions[0...-1].each { |exp| instructions << transform_expression(exp, used: false) }
        instructions << transform_expression(array_of_expressions.last, used: used) if array_of_expressions.last
        instructions.flatten
      end

      def transform_expression(exp, used:)
        case exp
        when Sexp
          method = "transform_#{exp.sexp_type}"
          send(method, exp, used: used)
        else
          raise "Unknown expression type: #{exp.inspect}"
        end
      end

      # INDIVIDUAL EXPRESSIONS = = = = =
      # (in alphabetical order)

      def transform_block(exp, used:)
        _, *body = exp
        transform_array_of_expressions(body, used: used)
      end

      def transform_call(exp, used:)
        _, receiver, message, *args = exp
        instructions = args.map { |arg| transform_expression(arg, used: true) }
        instructions << PushArgcInstruction.new(args.size)
        if receiver.nil?
          instructions << PushSelfInstruction.new
        else
          instructions << transform_expression(receiver, used: true)
        end
        instructions << SendInstruction.new(message)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_class(exp, used:)
        _, name, superclass, *body = exp
        instructions = [DefineClassInstruction.new(name: name, superclass: superclass)]
        instructions += transform_array_of_expressions(body, used: false)
        instructions << EndInstruction.new(:define_class)
        instructions << PushNilInstruction.new if used
        instructions
      end

      def transform_const(exp, used:)
        return [] unless used
        _, name = exp
        ConstFindInstruction.new(name)
      end

      def transform_defn(exp, used:)
        _, name, args, *body = exp
        arity = args.size - 1 # FIXME: way more complicated than this :-)
        instructions = [DefineMethodInstruction.new(name: name, arity: arity)] + transform_defn_args(args, used: true)
        instructions += transform_array_of_expressions(body, used: true)
        instructions << EndInstruction.new(:define_method)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_defn_args(exp, used:)
        return [] unless used
        _, *args = exp
        args.each_with_index.flat_map do |name, index|
          [PushArgInstruction.new(index), VariableSetInstruction.new(name)]
        end
      end

      def transform_if(exp, used:)
        _, condition, true_expression, false_expression = exp
        true_instructions = Array(transform_expression(true_expression, used: true))
        false_instructions = Array(transform_expression(false_expression, used: true))
        instructions = [
          transform_expression(condition, used: true),
          IfInstruction.new,
          true_instructions,
          ElseInstruction.new,
          false_instructions,
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_lasgn(exp, used:)
        _, name, value = exp
        instructions = [transform_expression(value, used: true), VariableSetInstruction.new(name)]
        instructions << VariableGetInstruction.new(name) if used
        instructions
      end

      def transform_lit(exp, used:)
        return [] unless used
        _, lit = exp
        case lit
        when Integer
          PushIntInstruction.new(lit)
        else
          raise "I don't yet know how to handle lit: #{lit.inspect}"
        end
      end

      def transform_lvar(exp, used:)
        return [] unless used
        _, name = exp
        VariableGetInstruction.new(name)
      end

      def transform_str(exp, used:)
        return [] unless used
        _, str = exp
        PushStringInstruction.new(str, str.size)
      end
    end
  end
end
