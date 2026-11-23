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
        transform.push_call("call i64 @nat_llvm_symbol(ptr #{transform.string(name)}, i64 #{name.bytesize})")
      end

      def generate(transform)
        sym = transform.intern(@name)
        transform.push("Value(#{sym})")
      end
    end
  end
end
