require_relative './base_instruction'

module Natalie
  class Compiler
    class RetryInstruction < BaseInstruction
      def initialize(id:)
        @id = id
      end

      def to_s
        'retry'
      end

      def generate(transform)
        transform.exec("#{retry_name} = true")
        transform.exec('continue')
        transform.push_nil
      end

      def execute(vm)
        raise 'todo'
      end

      private

      def retry_name
        "should_retry_#{@id}"
      end
    end
  end
end
