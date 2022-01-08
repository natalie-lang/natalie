require_relative './base_pass'

module Natalie
  class Compiler
    # Convert 'return' into 'throw LocalJumpError' where necessary.
    class Pass1r < BasePass
      def initialize(context)
        super
        @return_context = []
      end

      def process_begin_fn(exp)
        fn_type, fn_name, body = exp
        return_context(:fn) do |ctx|
          body = process(body)
          if ctx[:raises_local_jump_error]
            body = s(:rescue, body, s(:NAT_HANDLE_LOCAL_JUMP_ERROR, :env, :exception, :true))
          end
          exp.new(fn_type, fn_name, body)
        end
      end
      alias process_begin_fn process_begin_fn
      alias process_class_fn process_begin_fn
      alias process_def_fn process_begin_fn
      alias process_module_fn process_begin_fn

      def process_return(exp)
        _, value = exp
        return_context = @return_context.last

        if return_context[:type] == :fn
          exp.new(:c_return, process(value) || s(:nil))
        else
          @return_context.select { |c| c[:type] == :fn }.last[:raises_local_jump_error] = true
          exp.new(:raise_local_jump_error, :env, process(value), s(:l, 'LocalJumpErrorType::Return'))
        end
      end

      def process_iter(exp)
        _, block_fn, declare_block, call = exp
        return_context(:iter) { exp.new(:block, process(block_fn), process(declare_block), process(call)) }
      end

      def process_rescue(exp)
        _, try_body, rescue_body = exp
        rescue_body = process(rescue_body)
        exp.new(:rescue, process(try_body), s(:block, s(:NAT_RERAISE_LOCAL_JUMP_ERROR, :env, :exception), rescue_body))
      end

      private

      def return_context(type)
        ctx = { type: type, raises_local_jump_error: false }
        @return_context << ctx
        result = yield(ctx)
        @return_context.pop
        result
      end
    end
  end
end
