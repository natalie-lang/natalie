module Natalie
  class Compiler
    class Rescue
      def initialize(pass)
        @pass = pass
      end

      def transform(exp)
        _, *rescue_exprs = exp
        body = shift_body(rescue_exprs)
        else_body = pop_else_body(rescue_exprs)

        # `retry_id` is used to tie a later RetryInstruction
        # back to this TryInstruction.
        try_instruction = TryInstruction.new
        retry_id = try_instruction.object_id

        [
          try_instruction,
          transform_body(body || Prism.nil_node),
          CatchInstruction.new,
          transform_catch_body(rescue_exprs, retry_id: retry_id),
          EndInstruction.new(:try),
          transform_else_body(else_body),
        ]
      end

      private

      def transform_catch_body(rescue_exprs, retry_id:)
        catch_body = rescue_exprs.reduce([]) do |instr, rescue_expr|
          if rescue_expr.sexp_type == :resbody
            _, match_array, variable_set, *rescue_body = rescue_expr

            # empty array, so let's rescue StandardError
            if match_array.elements.empty?
              match_array = match_array.copy(elements: [match_array.new(:const, :StandardError)])
            end

            # Wrap in a retry_context so any RetryInstructions inside know
            # which rescue they are retrying.
            rescue_instructions = @pass.retry_context(retry_id) do
              @pass.transform_body(rescue_body, used: true)
            end

            instr + [
              @pass.transform_expression(match_array, used: true),
              MatchExceptionInstruction.new,
              IfInstruction.new,
              transform_reference_node(variable_set),
              rescue_instructions,
              ElseInstruction.new(:if),
            ]
          else
            raise "unknown expr: #{rescue_expr.inspect}"
          end
        end

        catch_body << @pass.transform_expression(
          Prism.call_node(receiver: nil, name: :raise),
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

      def transform_reference_node(node)
        return [] if node.nil?

        instructions = [GlobalVariableGetInstruction.new(:$!)]

        case node
        when ::Prism::ClassVariableTargetNode
          instructions << ClassVariableSetInstruction.new(node.name)
        when ::Prism::ConstantTargetNode, ::Prism::ConstantPathTargetNode
          prepper = ConstPrepper.new(node, pass: @pass)
          instructions << [
            GlobalVariableGetInstruction.new(:$!),
            prepper.namespace,
            ConstSetInstruction.new(prepper.name)
          ]
        when ::Prism::GlobalVariableTargetNode
          instructions << GlobalVariableSetInstruction.new(node.name)
        when ::Prism::InstanceVariableTargetNode
          instructions << InstanceVariableSetInstruction.new(node.name)
        when ::Prism::LocalVariableTargetNode
          instructions << VariableSetInstruction.new(node.name)
        else
          raise "unhandled reference node for rescue: #{node.inspect}"
        end

        instructions
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
    end
  end
end
