require_relative './base_instruction'

module Natalie
  class Compiler
    class PushArgsInstruction < BaseInstruction
      def initialize(for_block:, min_count:, max_count:, spread: false, to_array: true, include_keyword_hash: true)
        @for_block = for_block
        @min_count = min_count
        @max_count = max_count
        @spread = spread
        @to_array = to_array
        @include_keyword_hash = include_keyword_hash
      end

      def to_s
        s = "push_args #{@min_count}..#{@max_count}"
        s << ' (for_block)' if @for_block
        s << ' (spread)' if @spread
        s << ' (include_keyword_hash)' if @include_keyword_hash
        s
      end

      def generate(transform)
        if @for_block
          transform.exec_and_push(
            :args,
            "args.to_array_for_block(env, #{@min_count}, #{@max_count || -1}, #{@spread ? 'true' : 'false'}, #{@include_keyword_hash ? 'true' : 'false'})",
          )
        elsif @to_array
          transform.exec_and_push(:args, "args.to_array(#{@include_keyword_hash ? 'true' : 'false'})")
        else
          transform.exec_and_push(:args, 'args', type: 'auto')
        end
      end

      def execute(vm)
        if @for_block && @max_count > 1 && vm.args.size == 1
          if vm.args.first.is_a?(Array)
            vm.push(vm.args.first.dup)
          elsif vm.args.first.respond_to?(:to_ary)
            vm.push(vm.args.first.to_ary)
          else
            vm.push(vm.args)
          end
        else
          vm.push(vm.args)
        end
      end

      def serialize(_)
        flags = 0
        [
          @for_block,
          @spread,
          @to_array,
          @include_keyword_hash,
          !@min_count.nil?,
          !@max_count.nil?,
        ].each_with_index { |flag, index| flags |= (1 << index) if flag }
        bytecode = [instruction_number, flags].pack('CC')
        bytecode << [@min_count].pack('w') unless @min_count.nil?
        bytecode << [@max_count].pack('w') unless @max_count.nil?
        bytecode
      end

      def self.deserialize(io, _)
        flags = io.getbyte
        for_block = flags[0] == 1
        spread = flags[1] == 1
        to_array = flags[2] == 1
        include_keyword_hash = flags[3] == 1
        has_min_count = flags[4] == 1
        has_max_count = flags[5] == 1
        min_count = io.read_ber_integer if has_min_count
        max_count = io.read_ber_integer if has_max_count
        new(for_block:, min_count:, max_count:, spread:, to_array:, include_keyword_hash:)
      end
    end
  end
end
