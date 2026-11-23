require_relative './base_instruction'

module Natalie
  class Compiler
    class ConstFindInstruction < BaseInstruction
      def initialize(name, strict:, failure_mode: 'ConstMissing', file: nil, line: nil)
        @name = name.to_sym
        @strict = strict
        @failure_mode = failure_mode
        raise 'None not acceptable failure_mode here' if @failure_mode == 'None'

        # source location info
        @file = file
        @line = line
      end

      attr_reader :name, :strict, :file, :line

      def to_s
        s = "const_find #{@name}"
        s << ' (strict)' if @strict
        s
      end

      def generate_llvm(transform)
        transform.set_location(@file, @line)
        namespace = transform.pop
        failure = { 'ConstMissing' => 0, 'Raise' => 1 }.fetch(@failure_mode)
        transform.push_call(
          "call i64 @nat_llvm_const(ptr %env, i64 #{namespace}, i64 %self, ptr #{transform.string(@name.to_s)}, i32 #{@strict ? 1 : 0}, i32 #{failure})",
        )
      end

      def generate(transform)
        transform.set_file(@file)
        transform.set_line(@line)

        namespace = transform.pop
        search_mode = @strict ? 'Strict' : 'NotStrict'
        args = [
          'env',
          namespace,
          'self',
          transform.intern(name),
          "Object::ConstLookupSearchMode::#{search_mode}",
          "Object::ConstLookupFailureMode::#{@failure_mode}",
        ]
        transform.exec_and_push(:const, "Object::const_find_with_autoload(#{args.join(', ')}).value()")
      end

      def raise_if_missing!
        @failure_mode = 'Raise'
      end
    end
  end
end
