require_relative './base_instruction'

module Natalie
  class Compiler
    class IsDefinedInstruction < BaseInstruction
      def initialize(type:)
        @type = type
      end

      def to_s
        "is_defined #{@type}"
      end

      def has_body?
        true
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(until_instruction: EndInstruction, expected_label: :is_defined)
        result = transform.temp('is_defined_result')

        code = ["Value #{result}"]
        transform.with_same_scope(body) do |t|
          code << 'try {'
          code << t.transform
          code << "#{result} = StringObject::create(#{@type.inspect})"
          code << "#{result}->freeze()"
          code << '} catch (ExceptionObject *) {'
          code << "#{result} = Value::nil()"
          code << '}'
        end

        transform.exec(code)
        transform.push(result)
      end

      def execute(vm)
        raise 'todo'
      end
    end
  end
end
