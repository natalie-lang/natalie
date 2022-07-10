require_relative './base_instruction'

module Natalie
  class Compiler2
    class PushStringInstruction < BaseInstruction
      def initialize(string, length)
        @string = string
        @length = length
      end

      def to_s
        "push_string #{@string.inspect}, #{@length}"
      end

      def generate(transform)
        if @string.empty?
          transform.exec_and_push('string', "Value(new StringObject)")
        else
          transform.exec_and_push('string', "Value(new StringObject(#{to_cpp}, (size_t)#{@length}))")
        end
      end

      def execute(vm)
        vm.push(@string.dup)
      end

      private

      def to_cpp
        chars = @string.to_s.bytes.map do |byte|
            case byte
            when 9
              "\\t"
            when 10
              "\\n"
            when 13
              "\\r"
            when 27
              "\\e"
            when 34
              "\\\""
            when 92
              "\\134"
            when 32..126
              byte.chr(Encoding::ASCII_8BIT)
            else
              "\\%03o" % byte
            end
          end
        '"' + chars.join + '"'
      end
    end
  end
end
