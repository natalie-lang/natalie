class BindingGen
  def initialize
    @bindings = {}
    @undefine_singleton_methods = []
  end

  # define a method on the Ruby class and link it to a method on the C++ class
  def binding(*args, **kwargs)
    b = Binding.new(*args, **kwargs)
    b.increment_name while @bindings[b.name]
    @bindings[b.name] = b
    b.write
  end

  # define a method on the Ruby *SINGLETON* class and link it to a method on the C++ class
  def singleton_binding(*args, **kwargs)
    binding(*args, **kwargs.update(singleton: true))
  end

  # define a method on the Ruby *SINGLETON* class and link it to a *STATIC* method on the C++ class
  def static_binding(*args, **kwargs)
    binding(*args, **kwargs.update(static: true))
  end

  # mark a method as undefined on the Ruby singleton class
  def undefine_singleton_method(rb_class, method)
    @undefine_singleton_methods << [rb_class, method]
  end

  def init
    puts 'void init_bindings(Env *env) {'
    @consts = {}
    @bindings.values.each do |binding|
      unless @consts[binding.rb_class]
        puts "    " + binding.get_object
        @consts[binding.rb_class] = true
      end
      puts "    #{binding.rb_class}->#{binding.define_method_name}(env, #{binding.rb_method.inspect}, #{binding.name});"
    end
    @undefine_singleton_methods.each do |rb_class, method|
      puts "    #{rb_class}->undefine_singleton_method(env, #{method.inspect});"
    end
    puts '}'
  end

  class Binding
    def initialize(rb_class, rb_method, cpp_class, cpp_method, argc:, pass_env:, pass_block:, return_type:, singleton: false, static: false)
      @rb_class = rb_class
      @rb_method = rb_method
      @cpp_class = cpp_class
      @cpp_method = cpp_method
      @argc = argc
      @pass_env = pass_env
      @pass_block = pass_block
      @return_type = return_type
      @singleton = singleton
      @static = static
      generate_name
    end

    attr_reader :rb_class, :rb_method, :cpp_class, :cpp_method, :argc, :pass_env, :pass_block, :return_type, :name

    def write
      if @static
        write_static_function
      else
        write_function
      end
    end

    def write_function
      puts <<-FUNC
Value *#{name}(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    #{argc_assertion}
    #{cpp_class} *self = #{as_type 'self_value'};
    auto return_value = self->#{cpp_method}(#{args_to_pass});
    #{return_code}
}\n
      FUNC
    end

    def write_static_function
      puts <<-FUNC
Value *#{name}(Env *env, Value *, ssize_t argc, Value **args, Block *block) {
    #{argc_assertion}
    auto return_value = #{cpp_class}::#{cpp_method}(#{args_to_pass});
    #{return_code}
}\n
      FUNC
    end

    def args_to_pass
      case argc
      when :any
        [env_arg, 'argc', 'args', block_arg].compact.join(', ')
      when Range
        if argc.end
          ([env_arg] + args + [block_arg]).compact.join(', ')
        else
          [env_arg, 'argc', 'args', block_arg].compact.join(', ')
        end
      when Integer
        ([env_arg] + args + [block_arg]).compact.join(', ')
      end
    end

    def define_method_name
      "define#{@singleton || @static ? '_singleton' : ''}_method"
    end

    def increment_name
      @name = @name.sub(/_binding(\d*)$/) { "_binding#{$1.to_i + 1}" }
    end

    def get_object
      if rb_class.start_with?('$')
        "Value *#{rb_class} = env->global_get(#{rb_class.inspect});"
      else
        "Value *#{rb_class} = env->Object()->const_find(env, #{rb_class.inspect});"
      end
    end

    private

    def argc_assertion
      case argc
      when :any
        ''
      when Range
        if argc.end
          "NAT_ASSERT_ARGC(#{argc.begin}, #{argc.end});"
        else
          "NAT_ASSERT_ARGC_AT_LEAST(#{argc.begin});"
        end
      when Integer
        "NAT_ASSERT_ARGC(#{argc});"
      else
        raise "Unknown argc: #{argc.inspect}"
      end
    end

    def env_arg
      if pass_env
        'env'
      end
    end

    def args
      (0...max_argc).map do |i|
        "argc > #{i} ? args[#{i}] : nullptr"
      end
    end

    def block_arg
      if pass_block
        'block'
      end
    end

    def as_type(value)
      if cpp_class == 'Value'
        value
      elsif cpp_class =~ /ParserValue|EnvValue|Module$/ # TODO: put the special cases in an array in a constant :-)
        underscored = cpp_class.gsub(/([a-z])([A-Z])/,'\1_\2').downcase
        "#{value}->as_#{underscored}_for_method_binding()"
      else
        underscored = cpp_class.sub(/Value/, '').gsub(/([a-z])([A-Z])/,'\1_\2').downcase
        "#{value}->as_#{underscored}()"
      end
    end

    def return_code
      case return_type
      when :bool
        'if (return_value) { return env->true_obj(); } else { return env->false_obj(); }'
      when :ssize_t, :int
        'return new IntegerValue { env, return_value };'
      when :Value
        'return return_value;'
      when :NullableValue
        'if (return_value) { return return_value; } else { return env->nil_obj(); }'
      when :StringValue
        'return return_value;'
      else
        raise "Unknown return type: #{return_type.inspect}"
      end
    end

    def max_argc
      if Range === argc
        argc.end
      else
        argc
      end
    end

    def generate_name
      @name = "#{cpp_class}_#{cpp_method}#{@singleton ? '_singleton' : ''}#{@static ? '_static' : ''}_binding"
    end
  end
end

puts '// DO NOT EDIT THIS FILE BY HAND!'
puts '// This file is generated by the lib/natalie/compiler/binding_gen.rb script.'
puts '// Run `make src/bindings.cpp` to regenerate this file.'
puts
puts '#include "natalie.hpp"'
puts
puts 'namespace Natalie {'
puts

gen = BindingGen.new

gen.static_binding('Array', '[]', 'ArrayValue', 'square_new', argc: :any, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', '+', 'ArrayValue', 'add', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', '-', 'ArrayValue', 'sub', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', '<<', 'ArrayValue', 'ltlt', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', '<=>', 'ArrayValue', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', 'eql?', 'ArrayValue', 'eql', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', '==', 'ArrayValue', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', '===', 'ArrayValue', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', '[]', 'ArrayValue', 'ref', argc: 1..2, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', '[]=', 'ArrayValue', 'refeq', argc: 2..3, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', 'any?', 'ArrayValue', 'any', argc: :any, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Array', 'each', 'ArrayValue', 'each', argc: 0, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Array', 'each_with_index', 'ArrayValue', 'each_with_index', argc: 0, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Array', 'empty?', 'ArrayValue', 'is_empty', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Array', 'filter', 'ArrayValue', 'select', argc: 0, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Array', 'first', 'ArrayValue', 'first', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', 'include?', 'ArrayValue', 'include', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', 'initialize', 'ArrayValue', 'initialize', argc: 0..2, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', 'inspect', 'ArrayValue', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', 'join', 'ArrayValue', 'join', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', 'last', 'ArrayValue', 'last', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', 'length', 'ArrayValue', 'size', argc: 0, pass_env: false, pass_block: false, return_type: :ssize_t)
gen.binding('Array', 'map', 'ArrayValue', 'map', argc: 0, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Array', 'pop', 'ArrayValue', 'pop', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', 'select', 'ArrayValue', 'select', argc: 0, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Array', 'shift', 'ArrayValue', 'shift', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', 'size', 'ArrayValue', 'size', argc: 0, pass_env: false, pass_block: false, return_type: :ssize_t)
gen.binding('Array', 'sort', 'ArrayValue', 'sort', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', 'to_a', 'ArrayValue', 'to_ary', argc: 0, pass_env: false, pass_block: false, return_type: :Value)
gen.binding('Array', 'to_ary', 'ArrayValue', 'to_ary', argc: 0, pass_env: false, pass_block: false, return_type: :Value)
gen.binding('Array', 'to_s', 'ArrayValue', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Array', 'sample', 'ArrayValue', 'sample', argc: 0, pass_env: true, pass_block: false, return_type: :Value)

gen.binding('BasicObject', '__send__', 'Value', 'send', argc: 1.., pass_env: true, pass_block: true, return_type: :Value)
gen.binding('BasicObject', '!', 'Value', 'is_falsey', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('BasicObject', '==', 'Value', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('BasicObject', 'equal?', 'Value', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('BasicObject', '!=', 'Value', 'neq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('BasicObject', 'instance_eval', 'Value', 'instance_eval', argc: 0..1, pass_env: true, pass_block: true, return_type: :Value)

gen.static_binding('Class', 'new', 'ClassValue', 'new_method', argc: 0..1, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Class', 'superclass', 'ClassValue', 'superclass', argc: 0, pass_env: false, pass_block: false, return_type: :NullableValue)

gen.static_binding('Encoding', 'list', 'EncodingValue', 'list', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Encoding', 'inspect', 'EncodingValue', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Encoding', 'name', 'EncodingValue', 'name', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Encoding', 'names', 'EncodingValue', 'names', argc: 0, pass_env: true, pass_block: false, return_type: :Value)

gen.singleton_binding('ENV', 'inspect', 'EnvValue', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.singleton_binding('ENV', '[]', 'EnvValue', 'ref', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.singleton_binding('ENV', '[]=', 'EnvValue', 'refeq', argc: 2, pass_env: true, pass_block: false, return_type: :Value)

gen.binding('Exception', 'backtrace', 'ExceptionValue', 'backtrace', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Exception', 'initialize', 'ExceptionValue', 'initialize', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Exception', 'inspect', 'ExceptionValue', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Exception', 'message', 'ExceptionValue', 'message', argc: 0, pass_env: true, pass_block: false, return_type: :Value)

gen.undefine_singleton_method('FalseClass', 'new')
gen.binding('FalseClass', 'inspect', 'FalseValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('FalseClass', 'to_s', 'FalseValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value)

gen.static_binding('File', 'expand_path', 'FileValue', 'expand_path', argc: 1..2, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('File', 'initialize', 'FileValue', 'initialize', argc: 1..2, pass_env: true, pass_block: true, return_type: :Value)

gen.undefine_singleton_method('Float', 'new')
gen.binding('Float', '%', 'FloatValue', 'mod', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', '*', 'FloatValue', 'mul', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', '**', 'FloatValue', 'pow', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', '+', 'FloatValue', 'add', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', '+@', 'FloatValue', 'uplus', argc: 0, pass_env: false, pass_block: false, return_type: :Value)
gen.binding('Float', '-', 'FloatValue', 'sub', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', '-@', 'FloatValue', 'uminus', argc: 0, pass_env: false, pass_block: false, return_type: :Value)
gen.binding('Float', '/', 'FloatValue', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', '<', 'FloatValue', 'lt', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Float', '<=', 'FloatValue', 'lte', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Float', '<=>', 'FloatValue', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', '==', 'FloatValue', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Float', '===', 'FloatValue', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Float', '>', 'FloatValue', 'gt', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Float', '>=', 'FloatValue', 'gte', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Float', 'abs', 'FloatValue', 'abs', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'angle', 'FloatValue', 'arg', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'arg', 'FloatValue', 'arg', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'ceil', 'FloatValue', 'ceil', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'coerce', 'FloatValue', 'coerce', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'divmod', 'FloatValue', 'divmod', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'eql?', 'FloatValue', 'eql', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Float', 'fdiv', 'FloatValue', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'finite?', 'FloatValue', 'is_finite', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Float', 'floor', 'FloatValue', 'floor', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'infinite?', 'FloatValue', 'is_infinite', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'inspect', 'FloatValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'magnitude', 'FloatValue', 'abs', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'modulo', 'FloatValue', 'mod', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'nan?', 'FloatValue', 'is_nan', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Float', 'negative?', 'FloatValue', 'is_negative', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Float', 'next_float', 'FloatValue', 'next_float', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'phase', 'FloatValue', 'arg', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'positive?', 'FloatValue', 'is_positive', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Float', 'prev_float', 'FloatValue', 'prev_float', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'quo', 'FloatValue', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'round', 'FloatValue', 'round', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'to_f', 'FloatValue', 'to_f', argc: 0, pass_env: false, pass_block: false, return_type: :Value)
gen.binding('Float', 'to_i', 'FloatValue', 'to_i', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'to_int', 'FloatValue', 'to_i', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'to_s', 'FloatValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'truncate', 'FloatValue', 'truncate', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'zero?', 'FloatValue', 'is_zero', argc: 0, pass_env: false, pass_block: false, return_type: :bool)

gen.static_binding('Hash', '[]', 'HashValue', 'square_new', argc: :any, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Hash', '==', 'HashValue', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Hash', '===', 'HashValue', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Hash', '[]', 'HashValue', 'ref', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Hash', '[]=', 'HashValue', 'refeq', argc: 2, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Hash', 'delete', 'HashValue', 'delete_key', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Hash', 'each', 'HashValue', 'each', argc: 0, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Hash', 'initialize', 'HashValue', 'initialize', argc: 0..1, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Hash', 'inspect', 'HashValue', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Hash', 'key?', 'HashValue', 'has_key', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Hash', 'include?', 'HashValue', 'has_key', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Hash', 'keys', 'HashValue', 'keys', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Hash', 'size', 'HashValue', 'size', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Hash', 'sort', 'HashValue', 'sort', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Hash', 'to_s', 'HashValue', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Hash', 'values', 'HashValue', 'values', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Hash', 'default', 'HashValue', 'default_value', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Hash', 'default_proc', 'HashValue', 'default_proc', argc: 0, pass_env: true, pass_block: false, return_type: :Value)

gen.undefine_singleton_method('Integer', 'new')
gen.binding('Integer', '%', 'IntegerValue', 'mod', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', '&', 'IntegerValue', 'bitwise_and', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', '*', 'IntegerValue', 'mul', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', '**', 'IntegerValue', 'pow', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', '+', 'IntegerValue', 'add', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', '-', 'IntegerValue', 'sub', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', '/', 'IntegerValue', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', '<=>', 'IntegerValue', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', '!=', 'Value', 'neq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Integer', '<', 'IntegerValue', 'lt', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Integer', '<=', 'IntegerValue', 'lte', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Integer', '>', 'IntegerValue', 'gt', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Integer', '>=', 'IntegerValue', 'gte', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Integer', '==', 'IntegerValue', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Integer', '===', 'IntegerValue', 'eqeqeq', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', 'abs', 'IntegerValue', 'abs', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', 'chr', 'IntegerValue', 'chr', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', 'coerce', 'IntegerValue', 'coerce', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', 'eql?', 'IntegerValue', 'eql', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Integer', 'even?', 'IntegerValue', 'is_even', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Integer', 'inspect', 'IntegerValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', 'odd?', 'IntegerValue', 'is_odd', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Integer', 'succ', 'IntegerValue', 'succ', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', 'times', 'IntegerValue', 'times', argc: 0, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Integer', 'to_i', 'IntegerValue', 'to_i', argc: 0, pass_env: false, pass_block: false, return_type: :Value)
gen.binding('Integer', 'to_s', 'IntegerValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', '|', 'IntegerValue', 'bitwise_or', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Integer', 'zero?', 'IntegerValue', 'is_zero', argc: 0, pass_env: false, pass_block: false, return_type: :bool)

gen.binding('IO', 'close', 'IoValue', 'close', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('IO', 'fileno', 'IoValue', 'fileno', argc: 0, pass_env: false, pass_block: false, return_type: :int)
gen.binding('IO', 'initialize', 'IoValue', 'initialize', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('IO', 'print', 'IoValue', 'print', argc: :any, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('IO', 'puts', 'IoValue', 'puts', argc: :any, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('IO', 'read', 'IoValue', 'read', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('IO', 'seek', 'IoValue', 'seek', argc: 1..2, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('IO', 'write', 'IoValue', 'write', argc: 1.., pass_env: true, pass_block: false, return_type: :Value)

gen.binding('Kernel', 'Array', 'KernelModule', 'Array', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'at_exit', 'KernelModule', 'at_exit', argc: 0, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Kernel', 'block_given?', 'KernelModule', 'block_given', argc: 0, pass_env: true, pass_block: true, return_type: :bool)
gen.binding('Kernel', 'class', 'KernelModule', 'klass_obj', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', '__dir__', 'KernelModule', 'cur_dir', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'define_singleton_method', 'KernelModule', 'define_singleton_method', argc: 1, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Kernel', 'dup', 'KernelModule', 'dup', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', '===', 'KernelModule', 'equal', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Kernel', 'eql?', 'KernelModule', 'equal', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Kernel', 'exit', 'KernelModule', 'exit', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'freeze', 'KernelModule', 'freeze_obj', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'get_usage', 'KernelModule', 'get_usage', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'hash', 'KernelModule', 'hash', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'inspect', 'KernelModule', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'instance_variable_get', 'KernelModule', 'instance_variable_get', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'instance_variable_set', 'KernelModule', 'instance_variable_set', argc: 2, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'instance_variables', 'KernelModule', 'ivars', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'is_a?', 'KernelModule', 'is_a', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Kernel', 'nil?', 'KernelModule', 'is_nil', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Kernel', 'lambda', 'KernelModule', 'lambda', argc: 0, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Kernel', 'loop', 'KernelModule', 'loop', argc: 0, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Kernel', '__method__', 'KernelModule', 'this_method', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'methods', 'KernelModule', 'methods', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'public_methods', 'KernelModule', 'methods', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'object_id', 'KernelModule', 'object_id', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'p', 'KernelModule', 'p', argc: :any, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'print', 'KernelModule', 'print', argc: :any, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'proc', 'KernelModule', 'proc', argc: 0, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Kernel', 'puts', 'KernelModule', 'puts', argc: :any, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'raise', 'KernelModule', 'raise', argc: 1..2, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'respond_to?', 'KernelModule', 'respond_to', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Kernel', 'send', 'Value', 'send', argc: 1.., pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Kernel', 'singleton_class', 'KernelModule', 'singleton_class_obj', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'sleep', 'KernelModule', 'sleep', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Kernel', 'tap', 'KernelModule', 'tap', argc: 0, pass_env: true, pass_block: true, return_type: :Value)

gen.binding('MatchData', 'size', 'MatchDataValue', 'size', argc: 0, pass_env: false, pass_block: false, return_type: :ssize_t)
gen.binding('MatchData', 'length', 'MatchDataValue', 'size', argc: 0, pass_env: false, pass_block: false, return_type: :ssize_t)
gen.binding('MatchData', 'to_s', 'MatchDataValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('MatchData', '[]', 'MatchDataValue', 'ref', argc: 1, pass_env: true, pass_block: false, return_type: :Value)

gen.binding('Module', '===', 'ModuleValue', 'eqeqeq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Module', 'alias_method', 'ModuleValue', 'alias_method', argc: 2, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Module', 'ancestors', 'ModuleValue', 'ancestors', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Module', 'attr', 'ModuleValue', 'attr_reader', argc: 1.., pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Module', 'attr_accessor', 'ModuleValue', 'attr_accessor', argc: 1.., pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Module', 'attr_reader', 'ModuleValue', 'attr_reader', argc: 1.., pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Module', 'attr_writer', 'ModuleValue', 'attr_writer', argc: 1.., pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Module', 'class_eval', 'ModuleValue', 'module_eval', argc: 0, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Module', 'const_defined?', 'ModuleValue', 'const_defined', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Module', 'define_method', 'ModuleValue', 'define_method', argc: 1, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Module', 'extend', 'ModuleValue', 'extend', argc: 1.., pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Module', 'include', 'ModuleValue', 'include', argc: 1.., pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Module', 'include?', 'ModuleValue', 'does_include_module', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Module', 'included_modules', 'ModuleValue', 'included_modules', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Module', 'inspect', 'ModuleValue', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Module', 'method_defined?', 'ModuleValue', 'is_method_defined', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Module', 'module_eval', 'ModuleValue', 'module_eval', argc: 0, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Module', 'name', 'ModuleValue', 'name', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Module', 'prepend', 'ModuleValue', 'prepend', argc: 1.., pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Module', 'private', 'ModuleValue', 'private_method', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Module', 'protected', 'ModuleValue', 'protected_method', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Module', 'public', 'ModuleValue', 'public_method', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)

gen.undefine_singleton_method('NilClass', 'new')
gen.binding('NilClass', 'inspect', 'NilValue', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('NilClass', 'to_a', 'NilValue', 'to_a', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('NilClass', 'to_i', 'NilValue', 'to_i', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('NilClass', 'to_s', 'NilValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value)

gen.binding('Object', 'nil?', 'Value', 'is_nil', argc: 0, pass_env: false, pass_block: false, return_type: :bool)

gen.singleton_binding('Parser', 'parse', 'ParserValue', 'parse', argc: 1, pass_env: true, pass_block: false, return_type: :Value)

gen.binding('Proc', 'initialize', 'ProcValue', 'initialize', argc: 0, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Proc', 'call', 'ProcValue', 'call', argc: :any, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Proc', 'lambda?', 'ProcValue', 'is_lambda', argc: 0, pass_env: false, pass_block: false, return_type: :bool)

gen.static_binding('Process', 'pid', 'ProcessModule', 'pid', argc: 0, pass_env: true, pass_block: false, return_type: :Value)

gen.binding('Range', 'initialize', 'RangeValue', 'initialize', argc: 2..3, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Range', 'begin', 'RangeValue', 'begin', argc: 0, pass_env: false, pass_block: false, return_type: :Value)
gen.binding('Range', 'first', 'RangeValue', 'begin', argc: 0, pass_env: false, pass_block: false, return_type: :Value)
gen.binding('Range', 'end', 'RangeValue', 'end', argc: 0, pass_env: false, pass_block: false, return_type: :Value)
gen.binding('Range', 'last', 'RangeValue', 'end', argc: 0, pass_env: false, pass_block: false, return_type: :Value)
gen.binding('Range', 'exclude_end?', 'RangeValue', 'exclude_end', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Range', 'to_a', 'RangeValue', 'to_a', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Range', 'each', 'RangeValue', 'each', argc: 0, pass_env: true, pass_block: true, return_type: :Value)
gen.binding('Range', 'inspect', 'RangeValue', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Range', '==', 'RangeValue', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Range', '===', 'RangeValue', 'eqeqeq', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Range', 'include?', 'RangeValue', 'eqeqeq', argc: 1, pass_env: true, pass_block: false, return_type: :Value)

gen.static_binding('Regexp', 'compile', 'RegexpValue', 'compile', argc: 1..2, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Regexp', '==', 'RegexpValue', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Regexp', '===', 'RegexpValue', 'eqeqeq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Regexp', '=~', 'RegexpValue', 'eqtilde', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Regexp', 'initialize', 'RegexpValue', 'initialize', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Regexp', 'inspect', 'RegexpValue', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Regexp', 'match', 'RegexpValue', 'match', argc: 1, pass_env: true, pass_block: false, return_type: :Value)

gen.binding('String', '*', 'StringValue', 'mul', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', '+', 'StringValue', 'add', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', '<<', 'StringValue', 'ltlt', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', '<=>', 'StringValue', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', '==', 'StringValue', 'eq', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('String', '===', 'StringValue', 'eq', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('String', '=~', 'StringValue', 'eqtilde', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', '[]', 'StringValue', 'ref', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'bytes', 'StringValue', 'bytes', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'chars', 'StringValue', 'chars', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'encode', 'StringValue', 'encode', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'encoding', 'StringValue', 'encoding', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'eql?', 'StringValue', 'eq', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('String', 'force_encoding', 'StringValue', 'force_encoding', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'index', 'StringValue', 'index', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'initialize', 'StringValue', 'initialize', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'inspect', 'StringValue', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'length', 'StringValue', 'length', argc: 0, pass_env: false, pass_block: false, return_type: :ssize_t)
gen.binding('String', 'ljust', 'StringValue', 'ljust', argc: 1..2, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'match', 'StringValue', 'match', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'ord', 'StringValue', 'ord', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'size', 'StringValue', 'size', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'split', 'StringValue', 'split', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'sub', 'StringValue', 'sub', argc: 2, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'succ', 'StringValue', 'successive', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'to_i', 'StringValue', 'to_i', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('String', 'to_s', 'StringValue', 'to_s', argc: 0, pass_env: false, pass_block: false, return_type: :Value)
gen.binding('String', 'to_str', 'StringValue', 'to_str', argc: 0, pass_env: false, pass_block: false, return_type: :Value)
gen.binding('String', 'to_sym', 'StringValue', 'to_sym', argc: 0, pass_env: true, pass_block: false, return_type: :Value)

gen.undefine_singleton_method('Symbol', 'new')
gen.binding('Symbol', '<=>', 'SymbolValue', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Symbol', 'id2name', 'SymbolValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Symbol', 'inspect', 'SymbolValue', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Symbol', 'to_proc', 'SymbolValue', 'to_proc', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Symbol', 'to_s', 'SymbolValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value)

gen.undefine_singleton_method('TrueClass', 'new')
gen.binding('TrueClass', 'inspect', 'TrueValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('TrueClass', 'to_s', 'TrueValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value)

gen.singleton_binding('$NAT_main_object', 'inspect', 'KernelModule', 'main_obj_inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Value)

gen.init

puts
puts '}'
