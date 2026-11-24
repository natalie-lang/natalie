require_relative './base_instruction'

module Natalie
  class Compiler
    class PushSymbolInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "push_symbol #{@name.inspect}"
      end

      def generate_llvm(transform)
        name = @name.to_s
        transform.push_call("call i64 @nat_llvm_symbol(ptr #{transform.symbol_name(name)}, i64 #{name.bytesize})")
      end
    end
  end
end
