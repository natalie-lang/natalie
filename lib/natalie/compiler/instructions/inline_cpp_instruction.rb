require_relative './base_instruction'
require_relative '../comptime_values'

module Natalie
  class Compiler
    class InlineCppInstruction < BaseInstruction
      include ComptimeValues

      def initialize(exp)
        super()
        @exp = exp
      end

      attr_reader :exp

      def to_s
        "inline_cpp #{exp.name}"
      end

      def generate_llvm(transform)
        name = exp.name
        args = exp.arguments&.arguments || []
        validate_inline_scope!(env) if name == :__inline__
        count = name == :__call__ ? args.size - 1 : 0
        operands = transform.pop(count)
        fn = transform.native_extensions.wrapper(self, count)
        transform.push_call("call i64 @#{fn}(ptr %env, i64 %self, ptr #{transform.argument_array(operands)})")
      end

      def generate(transform)
        if exp.is_a?(::Prism::CallNode)
          name = exp.name
          rest = exp.arguments&.arguments || []
        else
          raise "unexpected node passed to InlineCppInstruction: #{exp.inspect}"
        end

        generator = "generate_#{name.to_s.gsub(/__/, '')}"
        send(generator, transform, *rest)
      end

      private

      def generate_bind_method(transform, ruby_name, cpp_name = ruby_name, arity = -1)
        ruby_name = comptime_symbol(ruby_name)
        cpp_name = comptime_symbol(cpp_name)
        arity = arity.value unless arity.is_a?(Integer)
        transform.exec_and_push(
          :method,
          "Object::define_method(env, self, #{ruby_name.to_s.inspect}_s, #{cpp_name}, #{arity})",
        )
      end

      def generate_alloc_func(transform, cpp_name)
        cpp_name = comptime_symbol(cpp_name)
        transform.exec("self.as_class()->set_alloc_func(#{cpp_name});")
        transform.push_nil
      end

      def generate_bind_static_method(transform, ruby_name, cpp_name = ruby_name, arity = -1)
        ruby_name = comptime_symbol(ruby_name)
        cpp_name = comptime_symbol(cpp_name)
        arity = arity.value unless arity.is_a?(Integer)
        transform.exec_and_push(
          :method,
          "Object::define_method(env, self->klass(), #{ruby_name.to_s.inspect}_s, #{cpp_name}, #{arity})",
        )
      end

      def generate_cxx_flags(transform, flags)
        flags = comptime_string(flags)
        transform.add_cxx_flags(flags)
        transform.push_nil
      end

      def generate_call(transform, fn_name, *args)
        fn_name = comptime_string(fn_name)
        fn = transform.inline_functions.fetch(fn_name)

        cast_value_to_cpp =
          lambda do |_, type|
            value = transform.pop # Pass1 already did the work to push the value onto the stack
            case type
            when 'double'
              "#{value}.as_float()->to_double()"
            when 'int'
              "#{value}.integer().to_nat_int_t()"
            when 'bool'
              "#{value}.is_truthy()"
            when 'Value'
              value
            else
              raise "I don't yet know how to cast arg type #{type}"
            end
          end

        cast_value_from_cpp =
          lambda do |value, type|
            case type
            when 'double'
              "Value(FloatObject::create(#{value}))"
            when 'Value'
              value
            else
              raise "I don't yet know how to cast return type #{type}"
            end
          end

        casted_args = []

        if fn[:args][0] == 'Env *'
          # Push the env directly. This allows us to omit it from the __call__
          # macro call.
          casted_args << 'env'
        end

        args.each_with_index do |arg, index|
          index += 1 if fn[:args][0] == 'Env *'

          type = fn[:args][index]
          casted_args << cast_value_to_cpp.(arg, type)
        end

        call = "#{fn_name}(#{casted_args.join(', ')})"
        if fn[:return_type] == 'void'
          transform.exec("#{call};")
          transform.push_nil
        else
          transform.exec_and_push(:call_result, cast_value_from_cpp.(call, fn[:return_type]))
        end
      end

      def generate_constant(transform, name, type, value = name)
        name = comptime_string(name)
        type = comptime_string(type)
        value = comptime_string(value)

        code =
          case type
          when 'int', 'unsigned short'
            "Object::const_set(env, self, \"#{name}\"_s, Value::integer(#{value}))"
          when 'bigint'
            "Object::const_set(env, self, \"#{name}\"_s, Integer(BigInt(#{value})));"
          else
            raise "I don't yet know how to handle constant of type #{type.inspect}"
          end

        transform.exec("#ifdef #{value}")
        transform.exec(code)
        transform.exec('#endif')
        transform.push_nil
      end

      def generate_define_method(transform, name, args, body = nil)
        if body.nil?
          body = args
          args = nil
        end
        name = comptime_symbol(name)
        fn = transform.temp("defn_#{name}")
        output = []
        output << "static Value #{fn}(Env *env, Value self, Args &&args, Block *block) {"
        if args
          args = args.elements
          output << "args.ensure_argc_is(env, #{args.size});"
          args.each_with_index { |arg, i| output << "Value #{comptime_symbol(arg)} = args[#{i}];" }
        end
        output << comptime_string(body)
        output << '}'
        transform.top(fn, output)
        arity = args ? args.size : -1
        transform.exec("self.as_module()->define_method(env, #{transform.intern(name)}, #{fn}, #{arity})")
        transform.push(transform.intern(name))
      end

      def generate_function(transform, name, args, return_type)
        name = comptime_string(name)
        args = comptime_array_of_strings(args)
        return_type = comptime_string(return_type)
        transform.inline_functions[name] = { args: args, return_type: return_type }
        transform.push_nil
      end

      def generate_inline(transform, body)
        body = comptime_string(body)
        validate_inline_scope!(transform.env)
        transform.top(body, body)
        transform.push_nil
      end

      def validate_inline_scope!(scope)
        scope = scope.fetch(:outer) while scope[:hoist]
        return if scope[:outer].nil? || scope[:main]

        raise CompileError,
              'scoped __inline__ has been retired; use __function__/__call__ for native helpers or __define_method__ for native methods'
      end

      def generate_internal_inline_code(transform, body)
        body = comptime_string(body)
        transform.top(body, body)
        transform.push_nil
      end

      def generate_ld_flags(transform, flags)
        flags = comptime_string(flags)
        transform.add_ld_flags(flags)
        transform.push_nil
      end
    end
  end
end
