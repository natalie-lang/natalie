require_relative './base_instruction'

module Natalie
  class Compiler
    class TryInstruction < BaseInstruction
      def initialize(discard_catch_result: false)
        @discard_catch_result = discard_catch_result
      end

      def to_s
        s = 'try'
        s << ' (discard_catch_result)' if @discard_catch_result
        s
      end

      def has_body?
        true
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(until_instruction: CatchInstruction)
        catch_body = transform.fetch_block_of_instructions(expected_label: :try) # or else or ensure?
        result = transform.temp('try_result')
        code = []

        has_retry = catch_body.detect { |i| i.is_a?(RetryInstruction) }

        # hoisted variables need to be set to nil here
        (@env[:hoisted_vars] || {}).each do |_, var|
          code << "Value #{var.fetch(:name)} = NilObject::the()"
          var[:declared] = true
        end

        code << "Value #{result}"

        if has_retry
          code << "bool #{retry_name}"
          code << "do {"
          code << "#{retry_name} = false"
        end

        transform.normalize_stack do
          code << 'try {'
          transform.with_same_scope(body) do |t|
            code << t.transform("#{result} =")
          end

          code << 'GlobalEnv::the()->set_rescued(false);'
          code << '} catch(ExceptionObject *exception) {'

          code << 'auto exception_was = env->exception()'
          code << 'env->set_exception(exception)'

          transform.with_same_scope(catch_body) do |t|
            code << t.transform(@discard_catch_result ? nil : "#{result} =")
          end

          code << 'env->set_exception(exception_was)'
          code << 'GlobalEnv::the()->set_rescued(true)'

          code << '}'
        end

        if has_retry
          code << "} while (#{retry_name})"
        end

        transform.exec(code)
        transform.push(result)
      end

      def execute(vm)
        start_ip = vm.ip
        vm.skip_block_of_instructions(until_instruction: CatchInstruction)
        catch_ip = vm.ip
        vm.skip_block_of_instructions(expected_label: :try)
        end_ip = vm.ip
        begin
          vm.ip = start_ip
          vm.run
          vm.ip = end_ip
          vm.rescued = false
        rescue => e
          vm.rescued = true
          vm.global_variables[:$!] = e
          vm.ip = catch_ip
          vm.run
          vm.ip = end_ip
          vm.global_variables.delete(:$!)
        end
      end

      private

      def retry_name
        "should_retry_#{object_id}"
      end
    end
  end
end
