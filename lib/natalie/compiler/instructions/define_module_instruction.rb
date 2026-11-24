require_relative './base_instruction'

module Natalie
  class Compiler
    class DefineModuleInstruction < BaseInstruction
      def initialize(name:, is_private:, file:, line:)
        @name = name
        @is_private = is_private

        # source location info
        @file = file
        @line = line
      end

      def has_body?
        true
      end

      attr_reader :name, :file, :line

      def private?
        @is_private
      end

      def to_s
        s = "define_module #{@name}"
        s << ' (private)' if @is_private
        s
      end

      def generate_llvm(transform)
        transform.set_location(@file, @line)
        body = transform.fetch_block_of_instructions(expected_label: :define_module)
        fn = transform.function(body, scope: env)
        namespace = transform.pop
        superclass = 4
        transform.push_call(
          "call i64 @nat_llvm_define_type(ptr %env, i64 #{namespace}, i64 %self, ptr #{transform.string(@name.to_s)}, i64 #{superclass}, ptr #{fn}, i32 #{private? ? 1 : 0}, i32 1)",
        )
      end
    end
  end
end
