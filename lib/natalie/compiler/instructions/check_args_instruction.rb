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
        s << ' (has_keywords)' if @required_keywords.any?
        s << ' (args array on stack)' if @args_array_on_stack
        s
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

      def execute(vm)
        count = vm.args.size
        count -= 1 if vm.kwargs.any?
        case @positional
        when Integer
          if count != @positional
            raise ArgumentError, "wrong number of arguments (given #{count}, expected #{@positional}#{error_suffix})"
          end
        when Range
          unless @positional.cover?(count)
            raise ArgumentError,
                  "wrong number of arguments (given #{count}, expected #{@positional.inspect}#{error_suffix})"
          end
        else
          raise "unknown CheckArgsInstruction @positional type: #{@positional.inspect}"
        end
      end

      def serialize(rodata)
        needs_positional_range = @positional.is_a?(Range) && !@positional.end.nil?
        has_positional_splat = @positional.is_a?(Range) && @positional.end.nil?
        flags = 0
        [
          @args_array_on_stack,
          needs_positional_range,
          has_positional_splat,
          @has_keywords,
        ].each_with_index { |flag, index| flags |= (1 << index) if flag }
        positional = @positional.is_a?(Range) ? @positional.first : @positional
        bytecode = [instruction_number, flags, positional].pack('CCw')
        bytecode << [@positional.last].pack('w') if needs_positional_range
        bytecode << [@required_keywords.size].pack('w')
        @required_keywords.each do |keyword|
          position = rodata.add(keyword.to_s)
          bytecode << [position].pack('w')
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
        required_keywords = []
        io.read_ber_integer.times do
          position = io.read_ber_integer
          required_keywords << rodata.get(position, convert: :to_sym)
        end
        new(positional:, has_keywords:, required_keywords:, args_array_on_stack:)
      end

      private

      def cpp_keywords
        "{ #{@required_keywords.map { |kw| kw.to_s.inspect }.join ', '} }"
      end

      def error_suffix
        if @required_keywords.size == 1
          "; required keyword: #{@required_keywords.first}"
        elsif @required_keywords.any?
          "; required keywords: #{@required_keywords.join ', '}"
        end
      end
    end
  end
end
