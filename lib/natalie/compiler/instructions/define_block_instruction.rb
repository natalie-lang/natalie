require_relative './base_instruction'

module Natalie
  class Compiler
    class DefineBlockInstruction < BaseInstruction
      def initialize(arity:, for_lambda: false, has_return: false, parameters: [], file: nil, line: nil)
        @arity = arity
        @for_lambda = for_lambda
        @has_return = has_return
        @parameters = parameters
        @file = file
        @line = line
      end

      def has_body?
        true
      end

      def for_lambda? = !!@for_lambda
      def has_return? = !!@has_return

      attr_reader :arity, :parameters
      attr_accessor :has_return, :redo_point, :next_point

      def to_s
        s = 'define_block'
        s << ' (for_lambda)' if for_lambda?
        s << ' (has_return)' if has_return?
        s
      end

      def generate_llvm(transform)
        body = transform.fetch_block_of_instructions(expected_label: :define_block)
        point = transform.break_point([:next, env.object_id])
        fn =
          transform.function(
            body,
            scope: env,
            return_point: point,
            debug_name: '<block>',
            source_location: [@file || transform.source_path, @line || 1],
          )
        transform.push_call(
          "call i64 @nat_llvm_block(ptr %env, i64 %self, ptr #{fn}, i32 #{@arity}, i64 #{point}, i32 #{has_return? ? 1 : 0}, i64 #{@redo_point ? transform.break_point(@redo_point) : 0}, ptr #{transform.parameter_table(@parameters)}, ptr #{transform.string(@file || transform.source_path)}, i64 #{@line || 0})",
        )
      end
    end
  end
end
