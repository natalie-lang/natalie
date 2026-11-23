require_relative './base_instruction'

module Natalie
  class Compiler
    class ArrayConcatInstruction < BaseInstruction
      def to_s
        'array_concat'
      end

      def generate_llvm(transform)
        other = transform.pop
        transform.emit "call void @nat_llvm_array_concat(ptr %env, i64 #{transform.peek}, i64 #{other})"
      end

      def generate(transform)
        ary2 = transform.pop
        return if ary2 == 'Value::nil()'

        ary = transform.peek
        # NOTE: ArrayObject::push_splat() is better than concat() because it
        # can handle when the given value is not an array.
        transform.exec("if (!#{ary2}.is_nil()) {")
        transform.exec("  #{ary}.as_array()->push_splat(env, #{ary2})")
        transform.exec('}')
      end
    end
  end
end
