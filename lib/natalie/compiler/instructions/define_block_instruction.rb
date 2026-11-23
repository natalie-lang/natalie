require_relative './base_instruction'

module Natalie
  class Compiler
    class DefineBlockInstruction < BaseInstruction
      def initialize(arity:, for_lambda: false, has_return: false, parameters: [])
        @arity = arity
        @for_lambda = for_lambda
        @has_return = has_return
        @parameters = parameters
      end

      def has_body?
        true
      end

      def for_lambda? = !!@for_lambda
      def has_return? = !!@has_return

      attr_reader :arity, :parameters
      attr_accessor :has_return, :redo_point, :next_point

      def to_s
        s = 'define_block'
        s << ' (for_lambda)' if for_lambda?
        s << ' (has_return)' if has_return?
        s
      end

      def generate_llvm(transform)
        body = transform.fetch_block_of_instructions(expected_label: :define_block)
        point = transform.break_point([:next, env.object_id])
        fn = transform.function(body, scope: env, return_point: point)
        transform.push_call(
          "call i64 @nat_llvm_block(ptr %env, i64 %self, ptr #{fn}, i32 #{@arity}, i64 #{point}, i32 #{has_return? ? 1 : 0}, i64 #{@redo_point ? transform.break_point(@redo_point) : 0}, ptr #{transform.parameter_table(@parameters)})",
        )
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(expected_label: :define_block)
        fn = transform.temp('block')
        transform.with_new_scope(body) do |t|
          body = []
          body << "Value #{fn}(Env *env, Value self, Args &&args, Block *block) {"
          body << 'try {' if @redo_point || @next_point
          body << t.transform('return')
          if @redo_point || @next_point
            body << '} catch (ExceptionObject *exception) {'
            if @next_point
              body << "if (exception->is_local_jump_error_with_break_point(#{@next_point})) return exception->ivar_get(env, \"@exit_value\"_s);"
            end
            if @redo_point
              body << "if (exception->is_local_jump_error_with_break_point(#{@redo_point})) {"
              body << 'FiberObject::current()->set_redo_flag();'
              body << 'return Value::nil();'
              body << '}'
            end
            body << 'throw;'
            body << '}'
          end
          body << '}'
          transform.top(fn, body)
        end
        params_ref = @parameters.empty? ? 'nullptr' : transform.intern_param_table(@parameters)
        transform.push(
          "Block::create(*env, self, #{fn}, #{@arity}, #{has_return?}, Block::BlockType::Proc, #{params_ref})",
        )
      end
    end
  end
end
