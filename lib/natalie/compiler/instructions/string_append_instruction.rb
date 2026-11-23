require_relative './base_instruction'

module Natalie
  class Compiler
    class StringAppendInstruction < BaseInstruction
      def to_s
        'string_append'
      end

      def generate_llvm(transform)
        part = transform.pop
        transform.emit "call void @nat_llvm_string_append(ptr %env, i64 #{transform.peek}, i64 #{part})"
      end

      def generate(transform)
        new_part = transform.pop
        target = transform.peek
        transform.exec("#{target}.as_string()->append(#{new_part}.send(env, \"to_s\"_s));")
      end
    end
  end
end
