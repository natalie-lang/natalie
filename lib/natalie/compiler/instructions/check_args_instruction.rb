require_relative './base_instruction'

module Natalie
  class Compiler
    class CheckArgsInstruction < BaseInstruction
      def initialize(positional:, has_keywords:, required_keywords:, args_array_on_stack: false)
        @positional = positional
        @has_keywords = has_keywords
        @required_keywords = required_keywords
        @args_array_on_stack = args_array_on_stack
      end

      def to_s
        s = "check_args positional: #{@positional.inspect}"
        s << "; required_keywords: #{@required_keywords.join ', '}" if @required_keywords.any?
        s << ' (has_keywords)' if @has_keywords
        s << ' (args array on stack)' if @args_array_on_stack
        s
      end

      def generate_llvm(transform)
        min = @positional.is_a?(Range) ? @positional.begin : @positional
        max = @positional.is_a?(Range) ? @positional.end || -1 : @positional
        transform.emit "call void @nat_llvm_check_args(ptr %env, ptr %args, i64 #{min}, i64 #{max}, i32 #{@has_keywords ? 1 : 0}, i64 #{@required_keywords.size}, ptr #{transform.keyword_names(@required_keywords)})"
      end

      private

      def cpp_keywords
        "{ #{@required_keywords.map { |kw| kw.to_s.inspect }.join ', '} }"
      end
    end
  end
end
