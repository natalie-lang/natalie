require_relative './base_instruction'

module Natalie
  class Compiler2
    class TryInstruction < BaseInstruction
      def to_s
        'try'
      end

      def has_body?
        true
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(until_instruction: CatchInstruction)
        catch_body = transform.fetch_block_of_instructions(expected_label: :try) # or else or ensure?
        result = transform.temp('try_result')
        code = []
        code << "Value #{result}"
        code << 'try {'
        stack_sizes = []
        transform.with_same_scope(body) do |t|
          code << t.transform("#{result} =")
          stack_sizes << t.stack.size
        end
        code << '} catch(ExceptionObject *exception) {'
        code << 'env->global_set("$!"_s, exception);'
        transform.with_same_scope(catch_body) do |t|
          code << t.transform("#{result} =")
          stack_sizes << t.stack.size
        end
        code << '}'
        # truncate resulting stack to minimum size of either branch's stack above
        stack_sizes << transform.stack.size
        transform.stack[stack_sizes.min..] = []
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
        rescue => e
          vm.global_variables[:$!] = e
          vm.ip = catch_ip
          vm.run
          vm.ip = end_ip
          vm.global_variables.delete(:$!)
        end
      end
    end
  end
end
