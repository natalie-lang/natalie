require_relative './base_pass'

module Natalie
  class Compiler
    # Convert 'break' into 'return' or 'throw LocalJumpError' where necessary.
    class Pass1b < BasePass
      def initialize(context)
        super
        @break_context = []
      end

      def process_break(exp)
        _, value = exp
        break_context = @break_context.last
        containing_context = @break_context[-2]

        return process(exp.new(:invalid_break)) unless break_context

        type = break_context[:type]
        var_name = break_context[:var_name]

        case type
        when :fn
          return process(exp.new(:invalid_break))
        when :iter
          break_context[:raises_local_jump_error] = true
          exp.new(:raise_local_jump_error, :env, process(value), s(:l, 'LocalJumpErrorType::Break'))
        when :loop
          exp.new(:block, s(:set, var_name, process(value)), s(:c_break))
        when :rescue_do
          exp.new(
            :block,
            s(:set, var_name, process(value)),
            (containing_context && containing_context[:type] == :fn ? s(:block) : s(:add_break_flag, var_name)),
            s(:c_break),
          )
        else
          raise "unknown break context: #{type}"
        end
      end

      def process_bubble_break(exp)
        _, result_name = exp

        if @break_context.last
          s(
            :c_if,
            s(:has_break_flag, result_name),
            s(:block, s(:remove_break_flag, result_name), process(s(:break, s(:l, result_name), :drop_if_invalid))),
            result_name,
          )
        else
          s(:c_if, s(:has_break_flag, result_name), process(exp.new(:invalid_break)), result_name)
        end
      end

      def process_invalid_break(exp)
        exp.new(:raise, :env, s(:s, 'SyntaxError'), s(:s, 'Invalid break'))
      end

      def process_loop(exp)
        _, result_name, _body = exp
        break_context(:loop, result_name) { process_sexp(exp) }
      end

      def process_rescue_do(exp)
        _, _body, _retry_name, result_name = exp
        processed = break_context(:rescue_do, result_name) { process_sexp(exp) }
        result_name = temp('rescue_result')
        exp.new(:block, s(:declare, result_name, processed), process(s(:bubble_break, result_name)))
      end

      def process_iter(exp)
        _, block_fn, declare_block, call = exp
        break_context(:iter) do
          block_fn = process(block_fn)
          if @break_context.last[:raises_local_jump_error]
            call = s(:rescue, call, s(:NAT_HANDLE_LOCAL_JUMP_ERROR, :env, :exception, :false))
          end
          exp.new(:iter, block_fn, process(declare_block), process(call))
        end
      end

      def process_begin_fn(exp)
        break_context(:fn) { process_sexp(exp) }
      end
      alias process_begin_fn process_begin_fn
      alias process_class_fn process_begin_fn
      alias process_def_fn process_begin_fn
      alias process_module_fn process_begin_fn

      private

      def break_context(type, var_name = nil)
        @break_context << { type: type, var_name: var_name, raises_local_jump_error: false }
        result = yield
        @break_context.pop
        result
      end
    end
  end
end
