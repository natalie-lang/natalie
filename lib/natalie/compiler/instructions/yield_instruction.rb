require_relative './base_instruction'

module Natalie
  class Compiler
    class YieldInstruction < BaseInstruction
      def initialize(args_array_on_stack:, has_keyword_hash:)
        @args_array_on_stack = args_array_on_stack
        @has_keyword_hash = has_keyword_hash
      end

      attr_reader :args_array_on_stack,
                  :has_keyword_hash

      def to_s
        s = 'yield'
        s << ' (args array on stack)' if @args_array_on_stack
        s << ' (has keyword hash)' if @has_keyword_hash
        s
      end

      def generate(transform)
        if @args_array_on_stack
          args = "#{transform.pop}->as_array()"
          args_list = "Args(#{args}, #{@has_keyword_hash ? 'true' : 'false'})"
        else
          arg_count = transform.pop
          args = []
          arg_count.times { args.unshift transform.pop }
          args_list = "Args({ #{args.join(', ')} }, #{@has_keyword_hash ? 'true' : 'false'})"
        end
        transform.exec_and_push :yield, "NAT_RUN_BLOCK_FROM_ENV(env, #{args_list})"
      end

      def execute(vm)
        arg_count = vm.pop
        args = []
        arg_count.times { args.unshift vm.pop }
        raise LocalJumpError.new('no block given') unless vm.block
        result = vm.block.call(*args)
        vm.push result
      end

      def serialize(_)
        flags = 0
        [args_array_on_stack, has_keyword_hash].each_with_index do |flag, index|
          flags |= (1 << index) if flag
        end
        [
          instruction_number,
          flags,
        ].pack('CC')
      end

      def self.deserialize(io, _)
        flags = io.getbyte
        args_array_on_stack = flags[0] == 1
        has_keyword_hash = flags[1] == 1
        new(
          args_array_on_stack:,
          has_keyword_hash:,
        )
      end
    end
  end
end
