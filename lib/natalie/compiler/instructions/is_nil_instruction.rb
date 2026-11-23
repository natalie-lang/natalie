require_relative './base_instruction'

module Natalie
  class Compiler
    class IsNilInstruction < BaseInstruction
      def to_s
        'is_nil'
      end

      def generate_llvm(transform)
        condition = transform.value("icmp eq i64 #{transform.pop}, 4")
        transform.push(transform.value("select i1 #{condition}, i64 20, i64 0"))
      end

      def generate(transform)
        obj = transform.pop
        transform.exec_and_push(:is_nil, "bool_object(#{obj}.is_nil())")
      end
    end
  end
end
