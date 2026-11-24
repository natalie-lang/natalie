require_relative './base_instruction'

module Natalie
  class Compiler
    class FlipFlopInstruction < BaseInstruction
      def initialize(exclude_end:)
        @exclude_end = exclude_end
        super()
      end

      def has_body?
        true
      end

      attr_accessor :meta, :state_name

      def to_s
        "flip_flop #{@exclude_end ? 'exclude_end' : ''}"
      end

      def generate_llvm(transform)
        on_body = transform.fetch_block_of_instructions(until_instruction: ElseInstruction, expected_label: :flip_flop)
        off_body = transform.fetch_block_of_instructions(expected_label: :flip_flop)
        depth, index = transform.variable_address(self)
        active = transform.value("call i64 @nat_llvm_var_get(ptr %env, i64 #{depth}, i64 #{index})")
        active = transform.truthy(active)
        baseline = transform.stack.dup
        result = transform.slot('flip_flop_result')
        start, started, stop, yes, no, done = %w[start started stop yes no done].map { |s| transform.name("flip_#{s}") }
        transform.emit "br i1 #{active}, label %#{stop}, label %#{start}"
        transform.emit "#{start}:"
        transform.transform(on_body)
        condition = transform.truthy(transform.pop)
        transform.emit "br i1 #{condition}, label %#{started}, label %#{no}"
        transform.emit "#{started}:"
        transform.emit "call void @nat_llvm_var_set(ptr %env, i64 #{depth}, i64 #{index}, i64 2)"
        transform.emit "br label %#{@exclude_end ? yes : stop}"
        transform.emit "#{stop}:"
        transform.stack.replace(baseline)
        transform.transform(off_body)
        finished = transform.truthy(transform.pop)
        next_state = transform.value("select i1 #{finished}, i64 0, i64 2")
        transform.emit "call void @nat_llvm_var_set(ptr %env, i64 #{depth}, i64 #{index}, i64 #{next_state})"
        transform.emit "br label %#{yes}"
        [[yes, 2], [no, 0]].each do |label, value|
          transform.emit "#{label}:"
          transform.emit "store volatile i64 #{value}, ptr #{result}"
          transform.emit "br label %#{done}"
        end
        transform.emit "#{done}:"
        transform.stack.replace(baseline)
        transform.push(transform.value("load volatile i64, ptr #{result}"))
      end
    end
  end
end
