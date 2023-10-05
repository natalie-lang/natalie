module Natalie
  class Compiler
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

        _, *rescue_exprs = exp
        body = shift_body(rescue_exprs)
        else_body = pop_else_body(rescue_exprs)

        # `retry_id` is used to tie a later RetryInstruction
        # back to this TryInstruction.
        try_instruction = TryInstruction.new
        retry_id = try_instruction.object_id

        [
          try_instruction,
          transform_body(body || ::Prism::NilNode.new(nil)),
          CatchInstruction.new,
          transform_catch_body(rescue_exprs, retry_id: retry_id),
          EndInstruction.new(:try),
          transform_else_body(else_body),
        ]
      end

      private

      def transform_catch_body(rescue_exprs, retry_id:)
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
            _, exceptions_array, *rescue_body = rescue_expr
            variable_set, match_array = split_exceptions_array(exceptions_array)

            # Wrap in a retry_context so any RetryInstructions inside know
            # which rescue they are retrying.
            rescue_instructions = @pass.retry_context(retry_id) do
              @pass.transform_body(rescue_body, used: true)
            end

            instr + [
              @pass.transform_expression(match_array, used: true),
              MatchExceptionInstruction.new,
              IfInstruction.new,
              variable_set ? @pass.transform_expression(variable_set, used: false) : [],
              rescue_instructions,
              ElseInstruction.new(:if),
            ]
          else
            raise "unknown expr: #{rescue_expr.inspect}"
          end
        end

        catch_body << @pass.transform_expression(
          rescue_exprs.first.new(:call, nil, :raise),
          used: true
        )

        ends = [EndInstruction.new(:if)] * rescue_exprs.size

        catch_body + ends
      end

      def transform_body(body)
        raise 'expected a body' unless body

        @pass.transform_expression(body, used: true)
      end

      def transform_else_body(else_body)
        return [] unless else_body

        [
          PushRescuedInstruction.new,
          IfInstruction.new,
          # noop
          ElseInstruction.new(:if),
          PopInstruction.new, # we don't want the result of the try
          @pass.transform_expression(else_body, used: true),
          EndInstruction.new(:if),
        ]
      end

      def shift_body(rescue_exprs)
        if rescue_exprs.first.sexp_type != :resbody
          rescue_exprs.shift
        else
          nil
        end
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
          items = [array.new(:const, :StandardError)]
        end

        [variable_set, array.new(:array, *items)]
      end
    end
  end
end
