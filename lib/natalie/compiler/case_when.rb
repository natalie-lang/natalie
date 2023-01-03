module Natalie
  class Compiler
    class CaseWhen
      def initialize(pass)
        @pass = pass
      end

      def transform(exp, used:)
        _, value, *clauses, else_block = exp

        unless value
          return transform_when_without_value(exp, used: used)
        end

        instructions = []
        instructions << @pass.transform_expression(value, used: true)
        clauses.each do |clause|
          if clause.sexp_type != :when
            raise "Unexpected case clause: #{clause.sexp_type}"
          end
          instructions += transform_when(clause)
        end

        if else_block
          instructions << @pass.transform_expression(else_block, used: true)
        else
          instructions << PushNilInstruction.new
        end

        instructions << [EndInstruction.new(:if)] * clauses.size

        # The case value is never popped during comparison, so we have to pop it here.
        instructions << [SwapInstruction.new, PopInstruction.new]

        instructions << PopInstruction.new unless used
        instructions
      end

      private

      def transform_when_without_value(exp, used:)
        _, _, *clauses, else_block = exp
        instructions = []

        # glorified if-else
        clauses.each do |clause|
          # case
          # when a == b, b == c, c == d
          # =>
          # if (a == b || b == c || c == d)
          type, options, *body = clause

          raise "unexpected case clause: #{type}" unless type == :when

          # s(:array, option1, option2, ...)
          # =>
          # s(:or, option1, s(:or, option2, ...))
          options = options[2..].reduce(options[1]) { |prev, option| s(:or, prev, option) }
          instructions << @pass.transform_expression(options, used: true)
          instructions << IfInstruction.new
          instructions << @pass.transform_body(body, used: true)
          instructions << ElseInstruction.new(:if)
        end

        if else_block
          instructions << @pass.transform_expression(else_block, used: true)
        else
          instructions << PushNilInstruction.new
        end

        instructions << [EndInstruction.new(:if)] * clauses.size

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_when(exp)
        instructions = []

        # case a
        # when b, c, d
        # =>
        # if (b === a || c === a || d === a)
        _, options_array, *body = exp
        _, *options = options_array

        options.each do |option|
          # Splats are handled in the backend.
          # For C++, it's done in the is_case_equal() function.
          if option.sexp_type == :splat
            _, option = option
            is_splat = true
          else
            is_splat = false
          end

          option_instructions = @pass.transform_expression(option, used: true)
          instructions << option_instructions
          instructions << CaseEqualInstruction.new(is_splat: is_splat)
          instructions << IfInstruction.new
          instructions << PushTrueInstruction.new
          instructions << ElseInstruction.new(:if)
        end
        instructions << PushFalseInstruction.new
        instructions << [EndInstruction.new(:if)] * options.length
        instructions << IfInstruction.new
        instructions << @pass.transform_body(body, used: true)
        instructions << ElseInstruction.new(:if)

        instructions
      end
    end
  end
end
