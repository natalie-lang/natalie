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

      def serialize(rodata)
        needs_positional_range = @positional.is_a?(Range) && !@positional.end.nil?
        has_positional_splat = @positional.is_a?(Range) && @positional.end.nil?
        flags = 0
        [@args_array_on_stack, needs_positional_range, has_positional_splat, @keywords.any?].each_with_index do |flag, index|
          flags |= (1 << index) if flag
        end
        positional = @positional.is_a?(Range) ? @positional.first : @positional
        bytecode = [
                     instruction_number,
                     flags,
                     positional,
                   ].pack('CCw')
        if needs_positional_range
          bytecode << [@positional.last].pack('w')
        end
        if @keywords.any?
          bytecode << [@keywords.size].pack('w')
          @keywords.each do |keyword|
            position = rodata.add(keyword.to_s)
            bytecode << [position].pack('w')
          end
        end
        bytecode
      end

      def self.deserialize(io, rodata)
        flags = io.getbyte
        args_array_on_stack = flags[0] == 1
        needs_positional_range = flags[1] == 1
        has_positional_splat = flags[2] == 1
        has_keywords = flags[3] == 1
        positional = io.read_ber_integer
        if needs_positional_range
          positional2 = io.read_ber_integer
          positional = Range.new(positional, positional2)
        elsif has_positional_splat
          positional = Range.new(positional, nil)
        end
        keywords = []
        if has_keywords
          io.read_ber_integer.times do
            position = io.read_ber_integer
            keywords << rodata.get(position, convert: :to_sym)
          end
        end
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
