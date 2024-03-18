require_relative './base_instruction'

module Natalie
  class Compiler
    class CheckArgsInstruction < BaseInstruction
      def initialize(positional:, keywords:, args_array_on_stack: false)
        @positional = positional
        @keywords = keywords
        @args_array_on_stack = args_array_on_stack
      end

      def to_s
        s = "check_args positional: #{@positional.inspect}"
        s << "; keywords: #{@keywords.join ', '}" if @keywords.any?
        s << ' (args array on stack)' if @args_array_on_stack
        s
      end

      def generate(transform)
        case @positional
        when Integer
          transform.exec("args.ensure_argc_is(env, #{@positional}, #{cpp_keywords})")
        when Range
          if @positional.end
            transform.exec("args.ensure_argc_between(env, #{@positional.begin}, #{@positional.end}, #{cpp_keywords})")
          else
            transform.exec("args.ensure_argc_at_least(env, #{@positional.begin}, #{cpp_keywords})")
          end
        else
          raise "unknown CheckArgsInstruction @positional type: #{@positional.inspect}"
        end
      end

      def execute(vm)
        case @positional
        when Integer
          if vm.args.size != @positional
            raise ArgumentError, "wrong number of arguments (given #{vm.args.size}, expected #{@positional}#{error_suffix})"
          end
        when Range
          unless @positional.cover?(vm.args.size)
            raise ArgumentError, "wrong number of arguments (given #{vm.args.size}, expected #{@positional.inspect}#{error_suffix})"
          end
        else
          raise "unknown CheckArgsInstruction @positional type: #{@positional.inspect}"
        end
      end

      def serialize
        raise NotImplementedError, 'Support keyword arguments' if @keywords.any?

        positional = @positional.is_a?(Range) ? [@positional.first, @positional.last] : [@positional, @positional]
        [
          instruction_number,
          *positional,
          @args_array_on_stack ? 1 : 0,
        ].pack('CwwC')
      end

      def self.deserialize(io)
        positional = io.read_ber_integer
        positional2 = io.read_ber_integer
        positional = Range.new(positional, positional2) if positional != positional2
        keywords = [] # NATFIXME: Support keyword arguments
        args_array_on_stack = io.getbool
        new(positional:, keywords:, args_array_on_stack:)
      end

      private

      def cpp_keywords
        "{ #{@keywords.map { |kw| kw.to_s.inspect }.join ', ' } }"
      end

      def error_suffix
        if @keywords.size == 1
          "; required keyword: #{@keywords.first}"
        elsif @keywords.any?
          "; required keywords: #{@keywords.join ', '}"
        end
      end
    end
  end
end
