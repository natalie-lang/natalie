require_relative './base_instruction'

module Natalie
  class Compiler
    class StringToRegexpInstruction < BaseInstruction
      def initialize(options:, once: false, encoding: nil)
        @options = options || 0
        @once = once
        @encoding = encoding
      end

      def to_s
        str = "string_to_regexp (options=#{@options}, once=#{@once})"
        str += " (#{@encoding.name})" if @encoding
        str
      end

      def generate(transform)
        string = transform.pop
        if @once
          transform.exec_and_push(:regexp, "Value([&]() { static auto result = new RegexpObject(env, #{string}->as_string()->string(), #{@options}, #{encoding}); return result; }())");
        else
          transform.exec_and_push(:regexp, "Value(new RegexpObject(env, #{string}->as_string()->string(), #{@options}, #{encoding}))")
        end
      end

      def execute(vm)
        string = vm.pop
        options = @options
        if @encoding
          string = string.dup.force_encoding(@encoding)
          options |= Regexp::FIXEDENCODING
        end
        vm.push(Regexp.new(string, options))
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

      private

      def encoding
        case @encoding
        when Encoding::EUC_JP
          'EncodingObject::get(Encoding::EUC_JP)'
        else
          'nullptr';
        end
      end
    end
  end
end
