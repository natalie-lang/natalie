require_relative './base_instruction'
require_relative '../regexp_encoding'

module Natalie
  class Compiler
    class PushRegexpInstruction < BaseInstruction
      include StringToCpp
      include RegexpEncoding

      def initialize(regexp, encoding: nil)
        @regexp = regexp
        @encoding = encoding
      end

      def to_s
        str = "push_regexp #{@regexp.inspect}"
        str += " (#{@encoding.name})" if @encoding
        str
      end

      def generate(transform)
        transform.exec_and_push(
          :regexp,
          "Value(RegexpObject::literal(env, #{string_to_cpp(@regexp.source)}, #{@regexp.options}, #{encoding}))",
        )
      end
    end
  end
end
