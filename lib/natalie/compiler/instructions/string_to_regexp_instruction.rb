require_relative './base_instruction'
require_relative '../regexp_encoding'

module Natalie
  class Compiler
    class StringToRegexpInstruction < BaseInstruction
      include RegexpEncoding

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

      def generate_llvm(transform)
        transform.unsupported(self, 'once-only regexp interpolation') if @once
        transform.push_call(
          "call i64 @nat_llvm_string_regexp(ptr %env, i64 #{transform.pop}, i32 #{@options}, i32 #{@encoding == Encoding::EUC_JP ? 1 : 0})",
        )
      end

      def generate(transform)
        string = transform.pop
        if @once
          transform.exec_and_push(
            :regexp,
            "Value([&]() { static auto result = RegexpObject::create(env, #{string}.as_string()->string(), #{@options}, #{encoding}); return result; }())",
          )
        else
          transform.exec_and_push(
            :regexp,
            "Value(RegexpObject::create(env, #{string}.as_string()->string(), #{@options}, #{encoding}))",
          )
        end
      end
    end
  end
end
