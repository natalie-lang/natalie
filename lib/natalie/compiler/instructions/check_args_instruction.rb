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
        if @has_keywords || @required_keywords.any? || @args_array_on_stack
          transform.unsupported(self, 'keyword parameters')
        end
        min = @positional.is_a?(Range) ? @positional.begin : @positional
        max = @positional.is_a?(Range) ? @positional.end || -1 : @positional
        transform.emit "call void @nat_llvm_check_args(ptr %env, ptr %args, i64 #{min}, i64 #{max})"
      end

      def generate(transform)
        case @positional
        when Integer
          transform.exec("args.ensure_argc_is(env, #{@positional}, #{@has_keywords}, #{cpp_keywords})")
        when Range
          if @positional.end
            transform.exec(
              "args.ensure_argc_between(env, #{@positional.begin}, #{@positional.end}, #{@has_keywords}, #{cpp_keywords})",
            )
          else
            transform.exec("args.ensure_argc_at_least(env, #{@positional.begin}, #{@has_keywords}, #{cpp_keywords})")
          end
        else
          raise "unknown CheckArgsInstruction @positional type: #{@positional.inspect}"
        end
      end

      private

      def cpp_keywords
        "{ #{@required_keywords.map { |kw| kw.to_s.inspect }.join ', '} }"
      end
    end
  end
end
