require_relative './base_instruction'
require_relative '../regexp_encoding'

module Natalie
  class Compiler
    class StringToRegexpInstruction < BaseInstruction
      include RegexpEncoding

      def initialize(options:, encoding: nil)
        @options = options || 0
        @encoding = encoding
      end

      def to_s
        str = "string_to_regexp (options=#{@options})"
        str += " (#{@encoding.name})" if @encoding
        str
      end

      def generate_llvm(transform)
        transform.push_call(
          "call i64 @nat_llvm_string_regexp(ptr %env, i64 #{transform.pop}, i32 #{@options}, i32 #{@encoding == Encoding::EUC_JP ? 1 : 0})",
        )
      end

      def generate(transform)
        string = transform.pop
        result =
          transform.exec_and_push(
            :regexp,
            "Value(RegexpObject::create(env, #{string}.as_string()->string(), #{@options}, #{encoding}))",
          )
        transform.exec("#{result}.as_regexp()->freeze()")
      end
    end
  end
end
