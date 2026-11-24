require_relative './base_instruction'

module Natalie
  class Compiler
    class AutoloadConstInstruction < BaseInstruction
      def initialize(name:, path:)
        @name = name
        @path = path
      end

      def has_body?
        true
      end

      def to_s
        "autoload_const #{@name}, #{@path.inspect}"
      end

      def generate_llvm(transform)
        body = transform.fetch_block_of_instructions(expected_label: :autoload_const)
        fn = transform.function(body, scope: env)
        transform.emit "call void @nat_llvm_autoload(ptr %env, i64 %self, ptr #{transform.string(@name.to_s)}, ptr #{transform.string(@path)}, ptr #{fn})"
        transform.push('4')
      end
    end
  end
end
