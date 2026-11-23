require_relative './base_instruction'

module Natalie
  class Compiler
    class TryInstruction < BaseInstruction
      attr_reader :serial, :has_ensure

      def initialize(discard_catch_result: false, has_ensure: false)
        @discard_catch_result = discard_catch_result
        @has_ensure = has_ensure
        @serial = self.class.serial
      end

      def to_s
        s = 'try'
        s << ' (discard_catch_result)' if @discard_catch_result
        s << ' (ensure)' if @has_ensure
        s
      end

      def has_body?
        true
      end

      def generate_llvm(transform)
        body = transform.fetch_block_of_instructions(until_instruction: CatchInstruction)
        handler = transform.fetch_block_of_instructions(expected_label: :try)
        baseline = transform.stack.dup
        protected_fn, protected_size = transform.region(body, baseline)
        catch_fn, catch_size = transform.region(handler, baseline)
        frame = transform.argument_array(baseline)
        transform.stack.replace(baseline.take([protected_size, catch_size].min))
        transform.push_call(
          "call i64 @nat_llvm_try(ptr %env, i64 %self, ptr #{transform.function_args}, ptr #{transform.function_block}, ptr #{protected_fn}, ptr #{catch_fn}, ptr #{frame}, i32 #{@has_ensure ? 1 : 0}, i32 #{@discard_catch_result ? 1 : 0}, i64 #{transform.break_point([:retry, @serial])})",
        )
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(until_instruction: CatchInstruction)
        catch_body = transform.fetch_block_of_instructions(expected_label: :try) # or else or ensure?
        result = transform.temp('try_result')
        code = []

        has_retry = catch_body.detect { |i| i.is_a?(RetryInstruction) }

        # hoisted variables need to be set to nil here
        (@env[:hoisted_vars] || {}).each do |_, var|
          code << "Value #{var.fetch(:name)} = Value::nil()"
          var[:declared] = true
        end

        code << "Value #{result}"

        if has_retry
          code << "bool #{retry_name}"
          code << 'do {'
          code << "#{retry_name} = false"
        end

        transform.normalize_stack do
          code << 'try {'
          transform.with_same_scope(body) { |t| code << t.transform("#{result} =") }

          code << 'GlobalEnv::the()->set_rescued(false);'
          code << '} catch(ExceptionObject *exception) {'

          code << 'auto exception_was = env->exception()'
          # The Defer ensures $! is restored even when the catch body itself
          # throws (e.g. a `return` from inside rescue, which propagates via
          # LocalJumpError so a surrounding ensure can run).
          code << 'Defer restore_exception([&] { env->set_exception(exception_was); })'
          if @has_ensure
            # Ensure-only try: control-flow throws (LocalJumpError from
            # `return`, `break`, etc.) must not clobber $! while the ensure
            # body runs; only real Ruby exceptions become the new $!.
            code << 'if (exception->local_jump_error_type() == LocalJumpErrorType::None) env->set_exception(exception)'
          else
            code << 'env->set_exception(exception)'
          end

          transform.with_same_scope(catch_body) { |t| code << t.transform(@discard_catch_result ? nil : "#{result} =") }

          code << 'GlobalEnv::the()->set_rescued(true)'

          # For an ensure-only try the catch body just runs the ensure clause;
          # the original exception (whether a real Ruby exception or a
          # control-flow LocalJumpError) must propagate up after that.
          code << 'throw exception' if @has_ensure

          code << '}'
        end

        code << "} while (#{retry_name})" if has_retry

        transform.exec(code)
        transform.push(result)
      end

      @serial = 0

      def self.serial
        @serial += 1
      end

      private

      def retry_name
        "should_retry_#{serial}"
      end
    end
  end
end
