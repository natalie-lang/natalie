require_relative './base_instruction'

module Natalie
  class Compiler
    class StringToRegexpInstruction < BaseInstruction
      def initialize(options:, once: false)
        @options = options || 0
        @once = once
      end

      def to_s
        "string_to_regexp (options=#{@options}, once=#{@once})"
      end

      def generate(transform)
        string = transform.pop
        if @once
          transform.exec_and_push(:regexp, "Value([&]() { static auto result = new RegexpObject(env, #{string}->as_string()->string(), #{@options}); return result; }())");
        else
          transform.exec_and_push(:regexp, "Value(new RegexpObject(env, #{string}->as_string()->string(), #{@options}))")
        end
      end

      def execute(vm)
        string = vm.pop
        vm.push(Regexp.new(string, @options))
      end

      def serialize(_)
        [
          instruction_number,
          @options,
        ].pack('Cw')
      end

      def self.deserialize(io, _)
        options = io.read_ber_integer
        new(options:)
      end
    end
  end
end
