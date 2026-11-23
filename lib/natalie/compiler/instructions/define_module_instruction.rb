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
        body = transform.fetch_block_of_instructions(expected_label: :define_module)
        fn = transform.function(body, scope: env)
        namespace = transform.pop
        superclass = 4
        transform.push_call(
          "call i64 @nat_llvm_define_type(ptr %env, i64 #{namespace}, i64 %self, ptr #{transform.string(@name.to_s)}, i64 #{superclass}, ptr #{fn}, i32 #{private? ? 1 : 0}, i32 1)",
        )
      end

      def generate(transform)
        transform.set_file(@file)
        transform.set_line(@line)

        body = transform.fetch_block_of_instructions(expected_label: :define_module)

        fn = transform.temp("module_#{@name}")
        transform.with_new_scope(body) do |t|
          fn_code = []
          fn_code << "Value #{fn}(Env *env, Value self) {"
          fn_code << t.transform('return')
          fn_code << '}'
          transform.top(fn, fn_code)
        end

        namespace = transform.pop
        search_mode = private? ? 'StrictPrivate' : 'Strict'

        code = [
          "Object::define_module(env, #{namespace}, self, #{transform.intern(@name)}, Object::ConstLookupSearchMode::#{search_mode}, #{fn})",
        ]

        transform.exec_and_push(:result_of_define_module, code)
      end
    end
  end
end
