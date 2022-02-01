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
        [
          TryInstruction.new,
          @pass.transform_expression(body, used: true),
          CatchInstruction.new,
          catch_body(rescue_exprs),
          EndInstruction.new(:try),
        ]
      end

      private

      def catch_body(rescue_exprs)
        # [
        #   s(:resbody, s(:array, s(:const, :NoMethodError)),
        #     s(:lit, 2)),
        #   s(:resbody, s(:array, s(:const, :ArgumentError)),
        #     s(:lit, 3)))
        # ]
        catch_body = rescue_exprs.reduce([]) do |instr, rescue_expr|
          if rescue_expr.sexp_type == :resbody
            _, exceptions_array, rescue_body = rescue_expr
            exceptions_array << s(:const, :StandardError) if exceptions_array.size == 1 # empty array
            instr + [
              @pass.transform_expression(exceptions_array, used: true),
              MatchExceptionInstruction.new,
              IfInstruction.new,
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
    end
  end
end
