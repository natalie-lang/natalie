require_relative './base_instruction'

module Natalie
  class Compiler
    class DefineMethodInstruction < BaseInstruction
      def initialize(name:, arity:, file:, line:, break_point: nil, parameters: [])
        @name = name
        @arity = arity
        @break_point = break_point
        @parameters = parameters

        # source location info
        @file = file
        @line = line
      end

      def has_body?
        true
      end

      attr_reader :name, :arity, :file, :line, :parameters
      attr_accessor :break_point

      def to_s
        s = "define_method #{@name}"
        s << " (break point: #{break_point})" if break_point
        s
      end

      def generate_llvm(transform)
        klass = transform.pop
        body = transform.fetch_block_of_instructions(expected_label: :define_method)
        point = transform.break_point(@break_point || [:return, env.object_id])
        fn = transform.function(body, scope: env, return_point: point)
        transform.emit "call void @nat_llvm_define_method(ptr %env, i64 #{klass}, ptr #{transform.string(@name.to_s)}, ptr #{fn}, i32 #{@arity}, ptr #{transform.string(@file)}, i64 #{@line}, i64 #{point}, ptr #{transform.parameter_table(@parameters)})"
      end

      def generate(transform)
        transform.set_file(@file)
        transform.set_line(@line)

        klass = transform.pop
        body = transform.fetch_block_of_instructions(expected_label: :define_method)
        fn = transform.temp("defn_#{@name}")
        transform.with_new_scope(body) do |t|
          body = []
          body << "Value #{fn}(Env *env, Value self, Args &&args, Block *block) {"
          body << t.transform('return')
          body << '}'
          transform.top(fn, body)
        end
        params_ref = @parameters.empty? ? 'nullptr' : transform.intern_param_table(@parameters)
        transform.exec(
          "Object::define_method(env, #{klass}, #{transform.intern(@name)}, #{fn}, #{@arity}, #{@break_point.to_i}, #{params_ref})",
        )
      end
    end
  end
end
