require_relative './base_instruction'

module Natalie
  class Compiler
    class InlineCppInstruction < BaseInstruction
      def initialize(exp)
        super()
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

      def execute(_vm)
        raise 'todo'
      end

      private

      def generate_bind_method(transform, ruby_name, cpp_name = ruby_name)
        ruby_name = comptime_symbol(ruby_name)
        cpp_name = comptime_symbol(cpp_name)
        arity = -1 # FIXME: not sure how to calculate this without more info
        transform.exec_and_push(
          :method,
          "self->define_method(env, #{ruby_name.to_s.inspect}_s, #{cpp_name}, #{arity})",
        )
      end

      def generate_call(transform, fn_name, *args)
        fn_name = comptime_string(fn_name)
        fn = transform.inline_functions.fetch(fn_name)

        cast_value_to_cpp = lambda do |_, type|
          value = transform.pop # Pass1 already did the work to push the value onto the stack
          case type
          when 'double'
            "#{value}.as_double()"
          else
            raise "I don't yet know how to cast arg type #{type}"
          end
        end

        cast_value_from_cpp = lambda do |value, type|
          case type
          when 'double'
            "Value::floatingpoint(#{value})"
          else
            raise "I don't yet know how to cast return type #{type}"
          end
        end

        casted_args = args.each_with_index.map do |arg, index|
          type = fn[:args][index]
          cast_value_to_cpp.(arg, type)
        end

        transform.exec_and_push(
          :call_result,
          cast_value_from_cpp.(
            "#{fn_name}(#{casted_args.join(', ')})",
            fn[:return_type],
          ),
        )
      end

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
        transform.exec("self->as_module()->define_method(env, #{transform.intern(name)}, #{fn}, -1)")
      end

      def generate_function(transform, name, args, return_type)
        name = comptime_string(name)
        args = comptime_array_of_strings(args)
        return_type = comptime_string(return_type)
        transform.inline_functions[name] = {
          args: args,
          return_type: return_type,
        }
        transform.push('NilObject::the()')
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

      def generate_ld_flags(transform, flags)
        flags = comptime_string(flags)
        transform.add_ld_flags(flags)
        transform.push('NilObject::the()')
      end

      def comptime_array_of_strings(exp)
        unless exp.sexp_type == :array
          raise "Expected array at compile time, but got: #{exp.inspect}"
        end
        exp[1..].map do |item|
          comptime_string(item)
        end
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
