require_relative './base_pass'
require_relative './args'

module Natalie
  class Compiler2
    class Pass1 < BasePass
      def initialize(ast)
        @ast = ast
      end

      # pass used: true to leave the final result on the stack
      def transform(used: true)
        raise 'unexpected AST input' unless @ast.sexp_type == :block
        transform_block(@ast, used: used).flatten
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

      private

      def transform_array_of_expressions(array_of_expressions, used:)
        instructions = []
        array_of_expressions[0...-1].each { |exp| instructions << transform_expression(exp, used: false) }
        instructions << transform_expression(array_of_expressions.last, used: used) if array_of_expressions.last
        instructions.flatten
      end

      # INDIVIDUAL EXPRESSIONS = = = = =
      # (in alphabetical order)

      def transform_and(exp, used:)
        _, lhs, rhs = exp
        lhs_instructions = transform_expression(lhs, used: true)
        rhs_instructions = transform_expression(rhs, used: true)
        instructions = [
          lhs_instructions,
          DupInstruction.new,
          IfInstruction.new,
          PopInstruction.new,
          rhs_instructions,
          ElseInstruction.new,
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_array(exp, used:)
        _, *items = exp
        instructions = items.map { |item| transform_expression(item, used: true) }
        instructions << CreateArrayInstruction.new(count: items.size)
      end

      def transform_block(exp, used:)
        _, *body = exp
        transform_array_of_expressions(body, used: used)
      end

      def transform_call(exp, used:, with_block: false)
        _, receiver, message, *args = exp
        instructions = args.map { |arg| transform_expression(arg, used: true) }
        instructions << PushArgcInstruction.new(args.size)
        if receiver.nil?
          instructions << PushSelfInstruction.new
        else
          instructions << transform_expression(receiver, used: true)
        end
        instructions << SendInstruction.new(message, with_block: with_block)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_class(exp, used:)
        _, name, superclass, *body = exp
        instructions = []
        if superclass
          instructions << transform_expression(superclass, used: true)
        else
          instructions << ConstFindInstruction.new('Object')
        end
        instructions << DefineClassInstruction.new(name: name)
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
        instructions = []
        instructions << DefineMethodInstruction.new(name: name, arity: arity)
        instructions << transform_defn_args(args, used: true)
        instructions += transform_array_of_expressions(body, used: true)
        instructions << EndInstruction.new(:define_method)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_defn_args(exp, used:)
        return [] unless used
        _, *args = exp

        if args.any? { |arg| arg.is_a?(Sexp) || arg.start_with?('*') }
          return [PushArgsInstruction.new, Args.new(self).transform(exp)].flatten
        end

        args.each_with_index.flat_map do |name, index|
          [PushArgInstruction.new(index), VariableSetInstruction.new(name)]
        end
      end

      # TODO: might need separate logic?
      alias transform_block_args transform_defn_args

      def transform_false(_, used:)
        return [] unless used
        PushFalseInstruction.new
      end

      def transform_dot2(exp, used:)
        range_instructions(exp, used, exclude_end: false)
      end

      def transform_dot3(exp, used:)
        range_instructions(exp, used, exclude_end: true)
      end

      def range_instructions(exp, used, exclude_end:)
        _, beginning, ending = exp
        instructions = [
          transform_expression(ending || s(:nil), used: true),
          transform_expression(beginning || s(:nil), used: true),
          PushRangeInstruction.new(exclude_end),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_if(exp, used:)
        _, condition, true_expression, false_expression = exp
        true_instructions = transform_expression(true_expression || s(:nil), used: true)
        false_instructions = transform_expression(false_expression || s(:nil), used: true)
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

      def transform_iter(exp, used:)
        _, call, args, body = exp
        arity = args.size - 1 # FIXME: way more complicated than this :-)
        instructions = []
        instructions << DefineBlockInstruction.new(arity: arity)
        instructions << transform_defn_args(args, used: true)
        instructions << transform_expression(body, used: true)
        instructions << EndInstruction.new(:define_block)
        raise 'unexpected call' unless call.sexp_type == :call
        instructions << transform_call(call, used: used, with_block: true)
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
        lit_instructions(lit)
      end

      def lit_instructions(lit)
        case lit
        when Integer
          PushIntInstruction.new(lit)
        when Float
          PushFloatInstruction.new(lit)
        when Symbol
          PushSymbolInstruction.new(lit)
        when Range
          [
            lit_instructions(lit.end),
            lit_instructions(lit.begin),
            PushRangeInstruction.new(lit.exclude_end?),
          ]
        else
          raise "I don't yet know how to handle lit: #{lit.inspect}"
        end
      end

      def transform_lvar(exp, used:)
        return [] unless used
        _, name = exp
        VariableGetInstruction.new(name)
      end

      def transform_nil(_, used:)
        return [] unless used
        PushNilInstruction.new
      end

      def transform_or(exp, used:)
        _, lhs, rhs = exp
        lhs_instructions = transform_expression(lhs, used: true)
        rhs_instructions = transform_expression(rhs, used: true)
        instructions = [
          lhs_instructions,
          DupInstruction.new,
          IfInstruction.new,
          ElseInstruction.new,
          PopInstruction.new,
          rhs_instructions,
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_self(_, used:)
        return [] unless used
        PushSelfInstruction.new
      end

      def transform_str(exp, used:)
        return [] unless used
        _, str = exp
        PushStringInstruction.new(str, str.size)
      end

      def transform_true(_, used:)
        return [] unless used
        PushTrueInstruction.new
      end

    end
  end
end
