require_relative './base_instruction'

module Natalie
  class Compiler
    class WhileInstruction < BaseInstruction
      def initialize(pre:)
        @pre = pre
      end

      attr_reader :pre

      def to_s
        s = 'while'
        s << ' (post)' unless pre
        s
      end

      def has_body?
        true
      end

      class << self
        def next_result_name
          @result_id ||= 0
          "while_result#{@result_id += 1}"
        end
      end

      def result_name
        @result_name ||= self.class.next_result_name
      end

      def generate(transform)
        condition = transform.fetch_block_of_instructions(until_instruction: WhileBodyInstruction)
        body = transform.fetch_block_of_instructions(until_instruction: EndInstruction, expected_label: :while)
        code = []

        # hoisted variables need to be set to nil here
        (@env[:hoisted_vars] || {}).each do |_, var|
          code << "Value #{var.fetch(:name)} = NilObject::the()"
          var[:declared] = true
        end

        condition_name = transform.temp('while_condition')

        transform.with_same_scope(condition) do |t|
          code << "auto #{condition_name} = [&]() {"
          code << t.transform("return")
          code << '};'
        end

        code << "Value #{result_name} = NilObject::the()"

        transform.with_same_scope(body) do |t|
          if @pre
            code << "while (#{condition_name}()->is_truthy()) {"
            code << t.transform
            code << '}'
          else
            code << 'do {'
            code << t.transform
            code << "} while (#{condition_name}()->is_truthy())"
          end
        end

        transform.exec(code)
        transform.push(result_name)
      end

      def execute(vm)
        start_ip = vm.ip
        vm.skip_block_of_instructions(until_instruction: WhileBodyInstruction)
        body_ip = vm.ip
        vm.skip_block_of_instructions(until_instruction: EndInstruction, expected_label: :while)
        end_ip = vm.ip
        condition = -> {
          vm.ip = start_ip
          vm.run
          vm.ip = end_ip
          vm.pop
        }
        if @pre
          while condition.()
            vm.ip = body_ip
            result = vm.run
            vm.ip = end_ip
            if result == :break_out
              result = :halt
              break
            end
          end
        else
          begin
            vm.ip = body_ip
            result = vm.run
            vm.ip = end_ip
            if result == :break_out
              p result
              result = :halt
              break
            end
          end while condition.()
        end
        unless result == :halt
          result
        end
      end

      def serialize(_)
        [
          instruction_number,
          pre ? 1 : 0,
        ].pack('CC')
      end

      def self.deserialize(io, _)
        pre = io.getbool
        new(pre:)
      end
    end
  end
end
