require_relative './base_instruction'

module Natalie
  class Compiler2
    class InlineCppInstruction < BaseInstruction
      def initialize(exp)
        @exp = exp
      end

      attr_reader :exp

      def to_s
        'inline_cpp'
      end

      def generate(transform)
        _, _, name, *rest = exp
        generator = "generate_#{name.to_s.gsub(/__/, '')}"
        send(generator, transform, *rest)
      end

      def execute(vm)
        raise 'todo'
      end

      private

      def generate_constant(transform, name, type)
        name = comptime_string(name)
        type = comptime_string(type)
        case type
        when 'int', 'unsigned short'
          transform.push("Value::integer(#{name})")
        else
          raise "I don't yet know how to handle constant of type #{type.inspect}"
        end
      end

      def generate_define_method(transform, name, args, body = nil)
        if body.nil?
          body = args
          args = nil
        end
        name = comptime_symbol(name)
        fn = transform.temp("defn_#{name}")
        output = []
        output << "Value #{fn}(Env *env, Value self, Args args, Block *block) {"
        if args
          _, *args = args
          output << "args.ensure_argc_is(env, #{args.size});"
          args.each_with_index do |arg, i|
            output << "Value #{comptime_symbol(arg)} = args[#{i}];"
          end
        end
        output << comptime_string(body)
        output << '}'
        transform.top(output)
        transform.exec("self->as_module()->define_method(env, #{name.to_s.inspect}_s, #{fn}, -1)")
      end

      def generate_inline(transform, body)
        body = comptime_string(body)
        env = transform.env
        env = env.fetch(:outer) while env[:hoist]
        if env[:outer].nil?
          transform.top body
        else
          transform.exec body
        end
        transform.push('NilObject::the()')
      end

      def comptime_string(exp)
        unless exp.sexp_type == :str
          raise "Expected string at compile time, but got: #{exp.inspect}"
        end
        exp.last
      end

      def comptime_symbol(exp)
        unless exp.sexp_type == :lit && exp.last.is_a?(Symbol)
          raise "Expected symbol at compile time, but got: #{exp.inspect}"
        end
        exp.last
      end
    end
  end
end
