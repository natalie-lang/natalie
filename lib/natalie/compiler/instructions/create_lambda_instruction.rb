require_relative './base_instruction'

module Natalie
  class Compiler
    class CreateLambdaInstruction < BaseInstruction
      def to_s
        s = 'create_lambda'
        s << " (break point: #{break_point})" if break_point
        s
      end

      attr_accessor :break_point

      def generate(transform)
        block = transform.pop
        block_temp = transform.temp('block')
        transform.exec_and_push(:lambda, [
          "auto #{block_temp} = #{block}",
          "#{block_temp}->set_type(Block::BlockType::Lambda)",
          "Value(new ProcObject(#{block_temp}, #{@break_point || 0}))"
        ])
      end

      def execute(vm)
        if @break_point.nil?
          # The "block" on the stack is a lambda already,
          # and there's no need to rescue a break.
          return
        end

        block_lambda = vm.pop
        proc_wrapper_that_catches_break = lambda do |*args|
          begin
            block_lambda.call
          rescue LocalJumpError => exception
            break_point = exception.instance_variable_get(:@break_point)
            if break_point == @break_point
              exception.exit_value
            else
              raise
            end
          end
        end
        vm.push(proc_wrapper_that_catches_break)
      end
    end
  end
end
