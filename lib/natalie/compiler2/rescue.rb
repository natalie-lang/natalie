module Natalie
  class Compiler2
    class Rescue
      def initialize(pass)
        @pass = pass
      end

      def transform(exp)
        # s(:rescue,
        #   s(:lit, 1),
        #   s(:resbody, s(:array, s(:const, :NoMethodError)),
        #     s(:lit, 2)),
        #   s(:resbody, s(:array, s(:const, :ArgumentError)),
        #     s(:lit, 3)))
        _, body, *rescue_exprs = exp
        else_body = pop_else_body(rescue_exprs)
        [
          TryInstruction.new,
          @pass.transform_expression(body, used: true),
          transform_else_body(else_body),
          CatchInstruction.new,
          transform_catch_body(rescue_exprs),
          EndInstruction.new(:try),
        ]
      end

      private

      def transform_catch_body(rescue_exprs)
        # [
        #   s(:resbody,
        #     s(:array,
        #       s(:const, :NoMethodError),
        #       s(:lasgn, :e, s(:gvar, :$!))),
        #     s(:lit, 2)),
        #   s(:resbody, s(:array, s(:const, :ArgumentError)),
        #     s(:lit, 3))
        # ]

        catch_body = rescue_exprs.reduce([]) do |instr, rescue_expr|
          if rescue_expr.sexp_type == :resbody
            _, exceptions_array, rescue_body = rescue_expr
            variable_set, match_array = split_exceptions_array(exceptions_array)
            instr + [
              @pass.transform_expression(match_array, used: true),
              MatchExceptionInstruction.new,
              IfInstruction.new,
              variable_set ? @pass.transform_expression(variable_set, used: false) : [],
              @pass.transform_expression(rescue_body, used: true),
              ElseInstruction.new,
            ]
          else
            raise "unknown expr: #{rescue_expr.inspect}"
          end
        end

        catch_body << @pass.transform_expression(s(:call, nil, :raise), used: true)

        ends = [EndInstruction.new(:if)] * rescue_exprs.size

        catch_body + ends
      end

      def transform_else_body(else_body)
        return [] unless else_body

        [
          PopInstruction.new, # we don't want the result of the try
          @pass.transform_expression(else_body, used: true)
        ]
      end

      def pop_else_body(rescue_exprs)
        if rescue_exprs.last.sexp_type != :resbody
          rescue_exprs.pop
        else
          nil
        end
      end

      def split_exceptions_array(array)
        _, *items = array

        # rescue => e
        if items.any? && items.last.sexp_type == :lasgn
          variable_set = items.pop
        end

        # empty array, so let's rescue StandardError
        if items.none?
          items = [s(:const, :StandardError)]
        end

        [variable_set, s(:array, *items)]
      end
    end
  end
end
