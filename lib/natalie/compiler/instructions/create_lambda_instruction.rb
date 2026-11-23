require_relative './base_instruction'

module Natalie
  class Compiler
    class CreateLambdaInstruction < BaseInstruction
      def initialize(file:, line:)
        super()

        # source location info
        @file = file
        @line = line
      end

      def to_s
        s = 'create_lambda'
        s << " (break point: #{break_point})" if break_point
        s
      end

      attr_reader :file, :line

      attr_accessor :break_point

      def generate(transform)
        transform.set_file(@file)
        transform.set_line(@line)

        block = transform.pop
        block_temp = transform.temp('block')
        transform.exec_and_push(
          :lambda,
          [
            "auto #{block_temp} = #{block}",
            "#{block_temp}->set_type(Block::BlockType::Lambda)",
            "Value(ProcObject::create(#{block_temp}, #{@break_point || 0}))",
          ],
        )
      end
    end
  end
end
