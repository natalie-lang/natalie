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

      def generate_llvm(transform)
        source = @regexp.source
        transform.push_call(
          "call i64 @nat_llvm_regexp(ptr %env, ptr #{transform.string(source)}, i64 #{source.bytesize}, i32 #{@regexp.options}, i32 #{@encoding == Encoding::EUC_JP ? 1 : 0})",
        )
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
