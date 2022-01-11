class BindingGen
  def initialize
    @bindings = {}
    @undefine_methods = []
    @undefine_singleton_methods = []
  end

  # define a method on the Ruby class and link it to a method on the C++ class
  def binding(*args, **kwargs)
    b = Binding.new(*args, **kwargs)
    b.increment_name while @bindings[b.name]
    @bindings[b.name] = b
    b.write
  end

  # mark a method as undefined on the Ruby class
  def undefine_instance_method(rb_class, method)
    @undefine_methods << [rb_class, method]
  end

  # define a method on the Ruby *SINGLETON* class and link it to a method on the C++ class
  def singleton_binding(*args, **kwargs)
    binding(*args, **kwargs.update(singleton: true))
  end

  # define a method on the Ruby *SINGLETON* class and link it to a *STATIC* method on the C++ class
  def static_binding(*args, **kwargs)
    binding(*args, **kwargs.update(static: true))
  end

  # define a private method on the Ruby class and a public method on the Ruby *SINGLETON* class
  def module_function_binding(*args, **kwargs)
    binding(*args, **{ module_function: true, visibility: :private }.update(kwargs))
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
        puts '    ' + binding.get_object
        @consts[binding.rb_class] = true
      end
      puts "    #{binding.rb_class_as_c_variable}->#{binding.define_method_name}(env, #{binding.rb_method.inspect}_s, #{binding.name}, #{binding.arity});"
      if binding.module_function?
        puts "    #{binding.rb_class_as_c_variable}->module_function(env, #{binding.rb_method.inspect}_s);"
      end
      if binding.needs_to_set_visibility?
        puts "    #{binding.rb_class_as_c_variable}->#{binding.set_visibility_method_name}(env, #{binding.rb_method.inspect}_s);"
      end
    end
    @undefine_methods.each { |rb_class, method| puts "    #{rb_class}->undefine_method(env, #{method.inspect}_s);" }
    @undefine_singleton_methods.each do |rb_class, method|
      puts "    #{rb_class}->undefine_singleton_method(env, #{method.inspect}_s);"
    end
    puts '}'
  end

  class Binding
    def initialize(
      rb_class,
      rb_method,
      cpp_class,
      cpp_method,
      argc:,
      pass_env:,
      pass_block:,
      return_type:,
      module_function: false,
      singleton: false,
      static: false,
      pass_klass: false,
      visibility: :public
    )
      @rb_class = rb_class
      @rb_method = rb_method
      @cpp_class = cpp_class
      @cpp_method = cpp_method
      @argc = argc
      @module_function = module_function
      @pass_env = pass_env
      @pass_block = pass_block
      @pass_klass = pass_klass
      @return_type = return_type
      @singleton = singleton
      @static = static
      @visibility = visibility
      generate_name
    end

    attr_reader :rb_class,
                :rb_method,
                :cpp_class,
                :cpp_method,
                :argc,
                :pass_env,
                :pass_block,
                :pass_klass,
                :return_type,
                :name,
                :visibility

    def module_function?
      @module_function
    end

    def arity
      case argc
      when :any
        -1
      when Range
        (argc.begin * -1) - 1
      when Integer
        argc
      else
        raise "unknown argc: #{argc}"
      end
    end

    def write
      @static ? write_static_function : write_function
    end

    def write_function
      type = cpp_class == 'Object' ? 'Value ' : "#{cpp_class} *"
      puts <<-FUNC
Value #{name}(Env *env, Value self_value, size_t argc, Value *args, Block *block) {
    #{argc_assertion}
    #{type}self = #{as_type 'self_value'};
    auto return_value = self->#{cpp_method}(#{args_to_pass});
    #{return_code}
}\n
      FUNC
    end

    def write_static_function
      puts <<-FUNC
Value #{name}(Env *env, Value klass, size_t argc, Value *args, Block *block) {
    #{argc_assertion}
    auto return_value = #{cpp_class}::#{cpp_method}(#{args_to_pass});
    #{return_code}
}\n
      FUNC
    end

    def args_to_pass
      case argc
      when :any
        [env_arg, 'argc', 'args', block_arg, klass_arg].compact.join(', ')
      when Range
        if argc.end
          ([env_arg] + args + [block_arg, klass_arg]).compact.join(', ')
        else
          [env_arg, 'argc', 'args', block_arg, klass_arg].compact.join(', ')
        end
      when Integer
        ([env_arg] + args + [block_arg, klass_arg]).compact.join(', ')
      end
    end

    def define_method_name
      "define#{@singleton || @static ? '_singleton' : ''}_method"
    end

    def increment_name
      @name = @name.sub(/_binding(\d*)$/) { "_binding#{$1.to_i + 1}" }
    end

    GLOBAL_ENV_ACCESSORS = %w[Array Binding Class Float Hash Integer Module Object Regexp String Symbol main_obj]

    def get_object
      if GLOBAL_ENV_ACCESSORS.include?(rb_class)
        "Value #{rb_class} = GlobalEnv::the()->#{rb_class}();"
      else
        "Value #{rb_class_as_c_variable} = GlobalEnv::the()->Object()->#{rb_class.split('::').map { |c| "const_find(env, #{c.inspect}_s)" }.join('->')};"
      end
    end

    def rb_class_as_c_variable
      rb_class.split('::').last
    end

    def needs_to_set_visibility?
      module_function? ? visibility != :private : visibility != :public
    end

    def set_visibility_method_name
      case visibility
      when :private
        'private_method'
      when :protected
        'protected_method'
      else
        raise "Unknown visibility: #{visibility.inspect}"
      end
    end

    private

    def argc_assertion
      case argc
      when :any
        ''
      when Range
        if argc.end
          "env->ensure_argc_between(argc, #{argc.begin}, #{argc.end});"
        else
          "env->ensure_argc_at_least(argc, #{argc.begin});"
        end
      when Integer
        "env->ensure_argc_is(argc, #{argc});"
      else
        raise "Unknown argc: #{argc.inspect}"
      end
    end

    def env_arg
      'env' if pass_env
    end

    def args
      (0...max_argc).map { |i| "argc > #{i} ? args[#{i}] : nullptr" }
    end

    def block_arg
      'block' if pass_block
    end

    def klass_arg
      'klass->as_class()' if pass_klass
    end

    SPECIAL_CLASSES_WITHOUT_DEDICATED_TYPES = %w[EnvObject KernelModule ParserObject SexpObject]

    def as_type(value)
      if cpp_class == 'Object'
        value
      elsif SPECIAL_CLASSES_WITHOUT_DEDICATED_TYPES.include?(cpp_class)
        underscored = cpp_class.gsub(/([a-z])([A-Z])/, '\1_\2').downcase
        "#{value}->as_#{underscored}_for_method_binding()"
      else
        underscored = cpp_class.sub(/Object|Object/, '').gsub(/([a-z])([A-Z])/, '\1_\2').downcase
        "#{value}->as_#{underscored}()"
      end
    end

    def return_code
      case return_type
      when :bool
        "if (!return_value) return FalseObject::the();\n" + 'return TrueObject::the();'
      when :int
        'return Value::integer(return_value);'
      when :size_t
        'return IntegerObject::from_size_t(env, return_value);'
      when :c_str
        "if (!return_value) return NilObject::the();\n" + 'return new StringObject { return_value };'
      when :Object
        "if (!return_value) return NilObject::the();\n" + 'return return_value;'
      when :NullableValue
        "if (!return_value) return NilObject::the();\n" + 'return return_value;'
      when :StringObject
        "if (!return_value) return NilObject::the();\n" + 'return return_value;'
      when :String
        "if (!return_value) return NilObject::the();\n" + 'return new StringObject { *return_value };'
      else
        raise "Unknown return type: #{return_type.inspect}"
      end
    end

    def max_argc
      Range === argc ? argc.end : argc
    end

    def generate_name
      @name =
        "#{cpp_class}_#{cpp_method}#{@singleton ? '_singleton' : ''}#{@static ? '_static' : ''}_#{@visibility}_binding"
    end
  end
end

puts '// DO NOT EDIT THIS FILE BY HAND!'
puts '// This file is generated by the lib/natalie/compiler/binding_gen.rb script.'
puts
puts '#include "natalie.hpp"'
puts
puts 'namespace Natalie {'
puts

gen = BindingGen.new

# prettier-ignore
begin
  gen.static_binding('Array', '[]', 'ArrayObject', 'square_new', argc: :any, pass_env: true, pass_block: false, pass_klass: true, return_type: :Object)
  gen.static_binding('Array', 'try_convert', 'ArrayObject', 'try_convert', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', '+', 'ArrayObject', 'add', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', '-', 'ArrayObject', 'sub', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', '*', 'ArrayObject', 'multiply', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', '&', 'ArrayObject', 'intersection', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', '<<', 'ArrayObject', 'ltlt', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', '<=>', 'ArrayObject', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'eql?', 'ArrayObject', 'eql', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Array', '==', 'ArrayObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Array', '===', 'ArrayObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Array', '[]', 'ArrayObject', 'ref', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', '[]=', 'ArrayObject', 'refeq', argc: 2..3, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'any?', 'ArrayObject', 'any', argc: :any, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'append', 'ArrayObject', 'push', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'at', 'ArrayObject', 'at', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'assoc', 'ArrayObject', 'assoc', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'bsearch', 'ArrayObject', 'bsearch', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'bsearch_index', 'ArrayObject', 'bsearch_index', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'clear', 'ArrayObject', 'clear', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'collect', 'ArrayObject', 'map', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'collect!', 'ArrayObject', 'map_in_place', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'concat', 'ArrayObject', 'concat', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'compact', 'ArrayObject', 'compact', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'compact!', 'ArrayObject', 'compact_in_place', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'cycle', 'ArrayObject', 'cycle', argc: 0..1, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'deconstruct', 'ArrayObject', 'itself', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Array', 'delete', 'ArrayObject', 'delete_item', argc: 1, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'delete_at', 'ArrayObject', 'delete_at', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'delete_if', 'ArrayObject', 'delete_if', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'difference', 'ArrayObject', 'difference', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'dig', 'ArrayObject', 'dig', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'drop', 'ArrayObject', 'drop', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'drop_while', 'ArrayObject', 'drop_while', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'each', 'ArrayObject', 'each', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'each_index', 'ArrayObject', 'each_index', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'empty?', 'ArrayObject', 'is_empty', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Array', 'fetch', 'ArrayObject', 'fetch', argc: 1..2, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'fill', 'ArrayObject', 'fill', argc: 0..3, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'filter', 'ArrayObject', 'select', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'filter!', 'ArrayObject', 'select_in_place', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'find_index', 'ArrayObject', 'index', argc: 0..1, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'first', 'ArrayObject', 'first', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'flatten', 'ArrayObject', 'flatten', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'flatten!', 'ArrayObject', 'flatten_in_place', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'hash', 'ArrayObject', 'hash', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'include?', 'ArrayObject', 'include', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Array', 'index', 'ArrayObject', 'index', argc: 0..1, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'intersection', 'ArrayObject', 'intersection', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'initialize', 'ArrayObject', 'initialize', argc: 0..2, pass_env: true, pass_block: true, return_type: :Object, visibility: :private)
  gen.binding('Array', 'initialize_copy', 'ArrayObject', 'initialize_copy', argc: 1, pass_env: true, pass_block: false, return_type: :Object, visibility: :private)
  gen.binding('Array', 'insert', 'ArrayObject', 'insert', argc: 1.., pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'inspect', 'ArrayObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'join', 'ArrayObject', 'join', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'keep_if', 'ArrayObject', 'keep_if', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'last', 'ArrayObject', 'last', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'length', 'ArrayObject', 'size', argc: 0, pass_env: false, pass_block: false, return_type: :size_t)
  gen.binding('Array', 'map', 'ArrayObject', 'map', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'map!', 'ArrayObject', 'map_in_place', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'max', 'ArrayObject', 'max', argc: 0..1, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'min', 'ArrayObject', 'min', argc: 0..1, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'minmax', 'ArrayObject', 'minmax', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'none?', 'ArrayObject', 'none', argc: :any, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'one?', 'ArrayObject', 'one', argc: :any, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'pack', 'ArrayObject', 'pack', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'pop', 'ArrayObject', 'pop', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'product', 'ArrayObject', 'product', argc: :any, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'push', 'ArrayObject', 'push', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'rassoc', 'ArrayObject', 'rassoc', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'reject', 'ArrayObject', 'reject', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'reject!', 'ArrayObject', 'reject_in_place', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'replace', 'ArrayObject', 'initialize_copy', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'reverse', 'ArrayObject', 'reverse', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'reverse!', 'ArrayObject', 'reverse_in_place', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'reverse_each', 'ArrayObject', 'reverse_each', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'rindex', 'ArrayObject', 'rindex', argc: 0..1, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'rotate', 'ArrayObject', 'rotate', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'rotate!', 'ArrayObject', 'rotate_in_place', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'select', 'ArrayObject', 'select', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'select!', 'ArrayObject', 'select_in_place', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'shift', 'ArrayObject', 'shift', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'size', 'ArrayObject', 'size', argc: 0, pass_env: false, pass_block: false, return_type: :size_t)
  gen.binding('Array', 'slice', 'ArrayObject', 'ref', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'slice!', 'ArrayObject', 'slice_in_place', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'sort', 'ArrayObject', 'sort', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'sort!', 'ArrayObject', 'sort_in_place', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'sort_by!', 'ArrayObject', 'sort_by_in_place', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'sum', 'ArrayObject', 'sum', argc: :any, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'to_a', 'ArrayObject', 'to_ary_method', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Array', 'to_ary', 'ArrayObject', 'to_ary_method', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Array', 'to_h', 'ArrayObject', 'to_h', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'to_s', 'ArrayObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'union', 'ArrayObject', 'union_of', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'uniq!', 'ArrayObject', 'uniq_in_place', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'uniq', 'ArrayObject', 'uniq', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'unshift', 'ArrayObject', 'unshift', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', 'zip', 'ArrayObject', 'zip', argc: 0.., pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Array', 'values_at', 'ArrayObject', 'values_at', argc: 0.., pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Array', '|', 'ArrayObject', 'union_of', argc: 1, pass_env: true, pass_block: false, return_type: :Object)

  gen.binding('BasicObject', '__send__', 'Object', 'send', argc: 1.., pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('BasicObject', '!', 'Object', 'is_falsey', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('BasicObject', '==', 'Object', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('BasicObject', 'equal?', 'Object', 'equal', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('BasicObject', '!=', 'Object', 'neq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('BasicObject', 'instance_eval', 'Object', 'instance_eval', argc: 0..1, pass_env: true, pass_block: true, return_type: :Object)

  gen.undefine_instance_method('Class', 'module_function')
  gen.binding('Class', 'initialize', 'ClassObject', 'initialize', argc: 0..1, pass_env: true, pass_block: true, return_type: :Object, visibility: :private)
  gen.binding('Class', 'superclass', 'ClassObject', 'superclass', argc: 0, pass_env: true, pass_block: false, return_type: :NullableValue)
  gen.binding('Class', 'singleton_class?', 'ClassObject', 'is_singleton', argc: 0, pass_env: false, pass_block: false, return_type: :bool)

  gen.static_binding('Encoding', 'aliases', 'EncodingObject', 'aliases', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.static_binding('Encoding', 'list', 'EncodingObject', 'list', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Encoding', 'inspect', 'EncodingObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Encoding', 'name', 'EncodingObject', 'name', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Encoding', 'names', 'EncodingObject', 'names', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

  gen.singleton_binding('ENV', 'inspect', 'EnvObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.singleton_binding('ENV', '[]', 'EnvObject', 'ref', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.singleton_binding('ENV', '[]=', 'EnvObject', 'refeq', argc: 2, pass_env: true, pass_block: false, return_type: :Object)

  gen.binding('Exception', 'backtrace', 'ExceptionObject', 'backtrace', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Exception', 'initialize', 'ExceptionObject', 'initialize', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Exception', 'inspect', 'ExceptionObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Exception', 'message', 'ExceptionObject', 'message', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

  gen.undefine_singleton_method('FalseClass', 'new')
  gen.binding('FalseClass', '&', 'FalseObject', 'and_method', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('FalseClass', '|', 'FalseObject', 'or_method', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('FalseClass', '^', 'FalseObject', 'or_method', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('FalseClass', 'inspect', 'FalseObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('FalseClass', 'to_s', 'FalseObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

  gen.static_binding('Fiber', 'yield', 'FiberObject', 'yield', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Fiber', 'initialize', 'FiberObject', 'initialize', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Fiber', 'resume', 'FiberObject', 'resume', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Fiber', 'status', 'FiberObject', 'status', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

  gen.static_binding('File', 'directory?', 'FileObject', 'directory', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.static_binding('File', 'file?', 'FileObject', 'file', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.static_binding('File', 'exist?', 'FileObject', 'exist', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.static_binding('File', 'expand_path', 'FileObject', 'expand_path', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
  gen.static_binding('File', 'open', 'FileObject', 'open', argc: 1..2, pass_env: true, pass_block: true, return_type: :Object)
  gen.static_binding('File', 'unlink', 'FileObject', 'unlink', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('File', 'initialize', 'FileObject', 'initialize', argc: 1..2, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('File', 'path', 'FileObject', 'path', argc: 0, pass_env: false, pass_block: false, return_type: :String)

  gen.undefine_singleton_method('Float', 'new')
  gen.binding('Float', '%', 'FloatObject', 'mod', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', '*', 'FloatObject', 'mul', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', '**', 'FloatObject', 'pow', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', '+', 'FloatObject', 'add', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', '+@', 'FloatObject', 'uplus', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Float', '-', 'FloatObject', 'sub', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', '-@', 'FloatObject', 'uminus', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Float', '/', 'FloatObject', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', '<', 'FloatObject', 'lt', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Float', '<=', 'FloatObject', 'lte', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Float', '<=>', 'FloatObject', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', '==', 'FloatObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Float', '===', 'FloatObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Float', '>', 'FloatObject', 'gt', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Float', '>=', 'FloatObject', 'gte', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Float', 'abs', 'FloatObject', 'abs', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'angle', 'FloatObject', 'arg', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'arg', 'FloatObject', 'arg', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'ceil', 'FloatObject', 'ceil', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'coerce', 'FloatObject', 'coerce', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'divmod', 'FloatObject', 'divmod', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'eql?', 'FloatObject', 'eql', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Float', 'fdiv', 'FloatObject', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'finite?', 'FloatObject', 'is_finite', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Float', 'floor', 'FloatObject', 'floor', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'infinite?', 'FloatObject', 'is_infinite', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'inspect', 'FloatObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'magnitude', 'FloatObject', 'abs', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'modulo', 'FloatObject', 'mod', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'nan?', 'FloatObject', 'is_nan', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Float', 'negative?', 'FloatObject', 'is_negative', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Float', 'next_float', 'FloatObject', 'next_float', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'phase', 'FloatObject', 'arg', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'positive?', 'FloatObject', 'is_positive', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Float', 'prev_float', 'FloatObject', 'prev_float', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'quo', 'FloatObject', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'round', 'FloatObject', 'round', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'to_f', 'FloatObject', 'to_f', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Float', 'to_i', 'FloatObject', 'to_i', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'to_int', 'FloatObject', 'to_i', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'to_s', 'FloatObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'truncate', 'FloatObject', 'truncate', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Float', 'zero?', 'FloatObject', 'is_zero', argc: 0, pass_env: false, pass_block: false, return_type: :bool)

  gen.static_binding('GC', 'enable', 'GCModule', 'enable', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.static_binding('GC', 'disable', 'GCModule', 'disable', argc: 0, pass_env: false, pass_block: false, return_type: :bool)

  gen.static_binding('Hash', '[]', 'HashObject', 'square_new', argc: :any, pass_env: true, pass_block: false, pass_klass: true, return_type: :Object)
  gen.binding('Hash', '==', 'HashObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Hash', '===', 'HashObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Hash', '>=', 'HashObject', 'gte', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Hash', '>', 'HashObject', 'gt', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Hash', '<=', 'HashObject', 'lte', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Hash', '<', 'HashObject', 'lt', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Hash', '[]', 'HashObject', 'ref', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', '[]=', 'HashObject', 'refeq', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'clear', 'HashObject', 'clear', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'compact', 'HashObject', 'compact', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'compact!', 'HashObject', 'compact_in_place', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'compare_by_identity', 'HashObject', 'compare_by_identity', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'compare_by_identity?', 'HashObject', 'is_comparing_by_identity', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Hash', 'default', 'HashObject', 'get_default', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'default=', 'HashObject', 'set_default', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'default_proc', 'HashObject', 'default_proc', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'default_proc=', 'HashObject', 'set_default_proc', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'delete', 'HashObject', 'delete_key', argc: 1, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Hash', 'delete_if', 'HashObject', 'delete_if', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Hash', 'dig', 'HashObject', 'dig', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'each', 'HashObject', 'each', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Hash', 'each_pair', 'HashObject', 'each', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Hash', 'empty?', 'HashObject', 'is_empty', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Hash', 'eql?', 'HashObject', 'eql', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Hash', 'except', 'HashObject', 'except', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'fetch', 'HashObject', 'fetch', argc: 1..2, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Hash', 'fetch_values', 'HashObject', 'fetch_values', argc: :any, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Hash', 'hash', 'HashObject', 'hash', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'has_key?', 'HashObject', 'has_key', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Hash', 'has_value?', 'HashObject', 'has_value', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Hash', 'initialize', 'HashObject', 'initialize', argc: 0..1, pass_env: true, pass_block: true, return_type: :Object, visibility: :private)
  gen.binding('Hash', 'initialize_copy', 'HashObject', 'replace', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'inspect', 'HashObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'include?', 'HashObject', 'has_key', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Hash', 'keep_if', 'HashObject', 'keep_if', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Hash', 'key?', 'HashObject', 'has_key', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Hash', 'keys', 'HashObject', 'keys', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'length', 'HashObject', 'size', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'member?', 'HashObject', 'has_key', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Hash', 'merge', 'HashObject', 'merge', argc: :any, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Hash', 'merge!', 'HashObject', 'merge_in_place', argc: :any, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Hash', 'replace', 'HashObject', 'replace', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'rehash', 'HashObject', 'rehash', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'size', 'HashObject', 'size', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'slice', 'HashObject', 'slice', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'store', 'HashObject', 'refeq', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'to_h', 'HashObject', 'to_h', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Hash', 'to_hash', 'HashObject', 'to_hash', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'to_s', 'HashObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Hash', 'update', 'HashObject', 'merge_in_place', argc: :any, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Hash', 'value?', 'HashObject', 'has_value', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Hash', 'values', 'HashObject', 'values', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

  gen.undefine_singleton_method('Integer', 'new')
  gen.binding('Integer', '%', 'IntegerObject', 'mod', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', '&', 'IntegerObject', 'bitwise_and', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', '*', 'IntegerObject', 'mul', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', '**', 'IntegerObject', 'pow', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', '+', 'IntegerObject', 'add', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', '-', 'IntegerObject', 'sub', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', '-@', 'IntegerObject', 'negate', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', '~', 'IntegerObject', 'complement', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', '/', 'IntegerObject', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', '<=>', 'IntegerObject', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', '!=', 'Object', 'neq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Integer', '<', 'IntegerObject', 'lt', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Integer', '<=', 'IntegerObject', 'lte', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Integer', '>', 'IntegerObject', 'gt', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Integer', '>=', 'IntegerObject', 'gte', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Integer', '==', 'IntegerObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Integer', '===', 'IntegerObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Integer', 'abs', 'IntegerObject', 'abs', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'chr', 'IntegerObject', 'chr', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'ceil', 'IntegerObject', 'ceil', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'coerce', 'IntegerObject', 'coerce', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'denominator', 'IntegerObject', 'denominator', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'eql?', 'IntegerObject', 'eql', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Integer', 'even?', 'IntegerObject', 'is_even', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Integer', 'floor', 'IntegerObject', 'floor', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'gcd', 'IntegerObject', 'gcd', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'inspect', 'IntegerObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'magnitude', 'IntegerObject', 'abs', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'modulo', 'IntegerObject', 'mod', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'next', 'IntegerObject', 'succ', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'numerator', 'IntegerObject', 'numerator', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'odd?', 'IntegerObject', 'is_odd', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Integer', 'ord', 'IntegerObject', 'ord', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'size', 'IntegerObject', 'size', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'succ', 'IntegerObject', 'succ', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'pred', 'IntegerObject', 'pred', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'times', 'IntegerObject', 'times', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Integer', 'to_f', 'IntegerObject', 'to_f', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'to_i', 'IntegerObject', 'to_i', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'to_int', 'IntegerObject', 'to_i', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'to_s', 'IntegerObject', 'to_s', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', '|', 'IntegerObject', 'bitwise_or', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Integer', 'zero?', 'IntegerObject', 'is_zero', argc: 0, pass_env: false, pass_block: false, return_type: :bool)

  gen.static_binding('IO', 'read', 'IoObject', 'read_file', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.static_binding('IO', 'write', 'IoObject', 'write_file', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('IO', 'close', 'IoObject', 'close', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('IO', 'closed?', 'IoObject', 'is_closed', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('IO', 'fileno', 'IoObject', 'fileno', argc: 0, pass_env: false, pass_block: false, return_type: :int)
  gen.binding('IO', 'initialize', 'IoObject', 'initialize', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('IO', 'print', 'IoObject', 'print', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('IO', 'puts', 'IoObject', 'puts', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('IO', 'read', 'IoObject', 'read', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('IO', 'seek', 'IoObject', 'seek', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('IO', 'write', 'IoObject', 'write', argc: 1.., pass_env: true, pass_block: false, return_type: :Object)

  gen.module_function_binding('Kernel', 'Array', 'KernelModule', 'Array', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.module_function_binding('Kernel', 'Float', 'KernelModule', 'Float', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
  gen.module_function_binding('Kernel', 'Hash', 'KernelModule', 'Hash', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.module_function_binding('Kernel', 'String', 'KernelModule', 'String', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'abort', 'KernelModule', 'abort_method', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'at_exit', 'KernelModule', 'at_exit', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Kernel', 'binding', 'KernelModule', 'binding', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'block_given?', 'KernelModule', 'block_given', argc: 0, pass_env: true, pass_block: true, return_type: :bool)
  gen.binding('Kernel', 'class', 'KernelModule', 'klass_obj', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'clone', 'KernelModule', 'clone', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', '__dir__', 'KernelModule', 'cur_dir', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'define_singleton_method', 'KernelModule', 'define_singleton_method', argc: 1, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Kernel', 'dup', 'KernelModule', 'dup', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', '===', 'KernelModule', 'equal', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Kernel', 'eql?', 'KernelModule', 'equal', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Kernel', 'exit', 'KernelModule', 'exit', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'extend', 'Object', 'extend', argc: 1.., pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'freeze', 'KernelModule', 'freeze_obj', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'frozen?', 'KernelModule', 'is_frozen', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Kernel', 'gets', 'KernelModule', 'gets', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'get_usage', 'KernelModule', 'get_usage', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'hash', 'KernelModule', 'hash', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'inspect', 'KernelModule', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'instance_variable_defined?', 'KernelModule', 'instance_variable_defined', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Kernel', 'instance_variable_get', 'KernelModule', 'instance_variable_get', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'instance_variable_set', 'KernelModule', 'instance_variable_set', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'instance_variables', 'KernelModule', 'instance_variables', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'is_a?', 'KernelModule', 'is_a', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Kernel', 'kind_of?', 'KernelModule', 'is_a', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Kernel', 'nil?', 'KernelModule', 'is_nil', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Kernel', 'lambda', 'KernelModule', 'lambda', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Kernel', 'loop', 'KernelModule', 'loop', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Kernel', '__method__', 'KernelModule', 'this_method', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'method', 'KernelModule', 'method', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'methods', 'KernelModule', 'methods', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'public_methods', 'KernelModule', 'methods', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'object_id', 'KernelModule', 'object_id', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'p', 'KernelModule', 'p', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'print', 'KernelModule', 'print', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'proc', 'KernelModule', 'proc', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Kernel', 'puts', 'KernelModule', 'puts', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'raise', 'KernelModule', 'raise', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'remove_instance_variable', 'KernelModule', 'remove_instance_variable', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'respond_to?', 'KernelModule', 'respond_to_method', argc: 1..2, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Kernel', 'send', 'Object', 'send', argc: 1.., pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Kernel', 'singleton_class', 'KernelModule', 'singleton_class_obj', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'sleep', 'KernelModule', 'sleep', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'spawn', 'KernelModule', 'spawn', argc: 1.., pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Kernel', 'tap', 'KernelModule', 'tap', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Kernel', 'to_s', 'KernelModule', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

  gen.undefine_singleton_method('MatchData', 'new')
  gen.undefine_singleton_method('MatchData', 'allocate')
  gen.binding('MatchData', 'size', 'MatchDataObject', 'size', argc: 0, pass_env: false, pass_block: false, return_type: :size_t)
  gen.binding('MatchData', 'length', 'MatchDataObject', 'size', argc: 0, pass_env: false, pass_block: false, return_type: :size_t)
  gen.binding('MatchData', 'to_s', 'MatchDataObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('MatchData', '[]', 'MatchDataObject', 'ref', argc: 1, pass_env: true, pass_block: false, return_type: :Object)

  gen.undefine_singleton_method('Method', 'new')
  gen.binding('Method', '==', 'MethodObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Method', 'inspect', 'MethodObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Method', 'owner', 'MethodObject', 'owner', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Method', 'arity', 'MethodObject', 'arity', argc: 0, pass_env: false, pass_block: false, return_type: :int)
  gen.binding('Method', 'call', 'MethodObject', 'call', argc: :any, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Method', 'to_proc', 'MethodObject', 'to_proc', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Method', 'unbind', 'MethodObject', 'unbind', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

  gen.binding('Module', 'initialize', 'ModuleObject', 'initialize', argc: 0, pass_env: true, pass_block: true, return_type: :Object, visibility: :private)
  gen.binding('Module', '===', 'ModuleObject', 'eqeqeq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Module', 'alias_method', 'ModuleObject', 'alias_method', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'ancestors', 'ModuleObject', 'ancestors', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'attr', 'ModuleObject', 'attr_reader', argc: 1.., pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'attr_accessor', 'ModuleObject', 'attr_accessor', argc: 1.., pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'attr_reader', 'ModuleObject', 'attr_reader', argc: 1.., pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'attr_writer', 'ModuleObject', 'attr_writer', argc: 1.., pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'class_eval', 'ModuleObject', 'module_eval', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Module', 'const_defined?', 'ModuleObject', 'const_defined', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Module', 'const_set', 'ModuleObject', 'const_set', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'define_method', 'ModuleObject', 'define_method', argc: 1..2, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Module', 'deprecate_constant', 'ModuleObject', 'deprecate_constant', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'extend', 'Object', 'extend', argc: 1.., pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'include', 'ModuleObject', 'include', argc: 1.., pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'include?', 'ModuleObject', 'does_include_module', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Module', 'included_modules', 'ModuleObject', 'included_modules', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'inspect', 'ModuleObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'instance_method', 'ModuleObject', 'instance_method', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'instance_methods', 'ModuleObject', 'instance_methods', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'method_defined?', 'ModuleObject', 'is_method_defined', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Module', 'module_eval', 'ModuleObject', 'module_eval', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Module', 'module_function', 'ModuleObject', 'module_function', argc: :any, pass_env: true, pass_block: false, return_type: :Object, visibility: :private)
  gen.binding('Module', 'name', 'ModuleObject', 'name', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'prepend', 'ModuleObject', 'prepend', argc: 1.., pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'private', 'ModuleObject', 'private_method', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'private_class_method', 'ModuleObject', 'private_class_method', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'private_constant', 'ModuleObject', 'private_constant', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'private_instance_methods', 'ModuleObject', 'private_instance_methods', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'protected', 'ModuleObject', 'protected_method', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'protected_instance_methods', 'ModuleObject', 'protected_instance_methods', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'public', 'ModuleObject', 'public_method', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'public_class_method', 'ModuleObject', 'public_class_method', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'public_constant', 'ModuleObject', 'public_constant', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'public_instance_methods', 'ModuleObject', 'public_instance_methods', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'public_instance_method', 'ModuleObject', 'public_instance_method', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'remove_method', 'ModuleObject', 'remove_method', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'to_s', 'ModuleObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Module', 'undef_method', 'ModuleObject', 'undef_method', argc: :any, pass_env: true, pass_block: false, return_type: :Object)

  gen.undefine_singleton_method('NilClass', 'new')
  gen.binding('NilClass', '&', 'NilObject', 'and_method', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('NilClass', '|', 'NilObject', 'or_method', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('NilClass', '^', 'NilObject', 'or_method', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('NilClass', '=~', 'NilObject', 'eqtilde', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('NilClass', 'inspect', 'NilObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('NilClass', 'to_a', 'NilObject', 'to_a', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('NilClass', 'to_f', 'NilObject', 'to_f', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('NilClass', 'to_h', 'NilObject', 'to_h', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('NilClass', 'to_i', 'NilObject', 'to_i', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('NilClass', 'to_s', 'NilObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

  gen.binding('Object', 'nil?', 'Object', 'is_nil', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Object', 'itself', 'Object', 'itself', argc: 0, pass_env: false, pass_block: false, return_type: :Object)

  gen.singleton_binding('Parser', 'parse', 'ParserObject', 'parse', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
  gen.singleton_binding('Parser', 'tokens', 'ParserObject', 'tokens', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)

  gen.binding('Proc', 'initialize', 'ProcObject', 'initialize', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Proc', 'arity', 'ProcObject', 'arity', argc: 0, pass_env: false, pass_block: false, return_type: :int)
  gen.binding('Proc', 'call', 'ProcObject', 'call', argc: :any, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Proc', 'lambda?', 'ProcObject', 'is_lambda', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Proc', 'to_proc', 'ProcObject', 'to_proc', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

  gen.static_binding('Process', 'pid', 'ProcessModule', 'pid', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

  gen.static_binding('Random', 'new_seed', 'RandomObject', 'new_seed', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.static_binding('Random', 'srand', 'RandomObject', 'srand', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Random', 'initialize', 'RandomObject', 'initialize', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object, visibility: :private)
  gen.binding('Random', 'rand', 'RandomObject', 'rand', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Random', 'seed', 'RandomObject', 'seed', argc: 0, pass_env: false, pass_block: false, return_type: :Object)

  gen.binding('Range', 'initialize', 'RangeObject', 'initialize', argc: 2..3, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Range', 'begin', 'RangeObject', 'begin', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Range', 'end', 'RangeObject', 'end', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Range', 'last', 'RangeObject', 'end', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('Range', 'exclude_end?', 'RangeObject', 'exclude_end', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Range', 'to_a', 'RangeObject', 'to_a', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Range', 'each', 'RangeObject', 'each', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Range', 'first', 'RangeObject', 'first', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Range', 'inspect', 'RangeObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Range', '==', 'RangeObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Range', '===', 'RangeObject', 'eqeqeq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Range', 'include?', 'RangeObject', 'include', argc: 1, pass_env: true, pass_block: false, return_type: :bool)

  gen.static_binding('Regexp', 'compile', 'RegexpObject', 'compile', argc: 1..2, pass_env: true, pass_block: false, pass_klass: true, return_type: :Object)
  gen.static_binding('Regexp', 'last_match', 'RegexpObject', 'last_match', argc: 0..1, pass_env: true, pass_block: false, pass_klass: false, return_type: :Object)
  gen.binding('Regexp', '==', 'RegexpObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Regexp', 'eql?', 'RegexpObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Regexp', '===', 'RegexpObject', 'eqeqeq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Regexp', '=~', 'RegexpObject', 'eqtilde', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Regexp', '~', 'RegexpObject', 'tilde', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Regexp', 'casefold?', 'RegexpObject', 'casefold', argc: 0, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Regexp', 'hash', 'RegexpObject', 'hash', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Regexp', 'initialize', 'RegexpObject', 'initialize', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object, visibility: :private)
  gen.binding('Regexp', 'inspect', 'RegexpObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Regexp', 'match', 'RegexpObject', 'match', argc: 1..2, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('Regexp', 'match?', 'RegexpObject', 'has_match', argc: 1..2, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Regexp', 'options', 'RegexpObject', 'options', argc: 0, pass_env: true, pass_block: false, return_type: :int)
  gen.binding('Regexp', 'source', 'RegexpObject', 'source', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Regexp', 'to_s', 'RegexpObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

  gen.static_binding('Sexp', 'from_array', 'SexpObject', 'from_array', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Sexp', 'new', 'SexpObject', 'new_method', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Sexp', 'inspect', 'SexpObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Sexp', 'sexp_type', 'SexpObject', 'first', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Sexp', 'file', 'SexpObject', 'file', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Sexp', 'file=', 'SexpObject', 'set_file', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Sexp', 'line', 'SexpObject', 'line', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Sexp', 'line=', 'SexpObject', 'set_line', argc: 1, pass_env: true, pass_block: false, return_type: :Object)

  gen.binding('String', '*', 'StringObject', 'mul', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', '+', 'StringObject', 'add', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', '+@', 'StringObject', 'uplus', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', '<<', 'StringObject', 'ltlt', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', '<=>', 'StringObject', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', '==', 'StringObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('String', '===', 'StringObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('String', '=~', 'StringObject', 'eqtilde', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', '[]', 'StringObject', 'ref', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'bytes', 'StringObject', 'bytes', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'bytesize', 'StringObject', 'bytesize', argc: 0, pass_env: false, pass_block: false, return_type: :size_t)
  gen.binding('String', 'chars', 'StringObject', 'chars', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'chr', 'StringObject', 'chr', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'concat', 'StringObject', 'concat', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'downcase', 'StringObject', 'downcase', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'each_char', 'StringObject', 'each_char', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('String', 'empty?', 'StringObject', 'is_empty', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('String', 'encode', 'StringObject', 'encode', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'encoding', 'StringObject', 'encoding', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'end_with?', 'StringObject', 'end_with', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('String', 'eql?', 'StringObject', 'eql', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('String', 'force_encoding', 'StringObject', 'force_encoding', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'gsub', 'StringObject', 'gsub', argc: 1..2, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('String', 'include?', 'StringObject', 'include', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('String', 'index', 'StringObject', 'index', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'initialize', 'StringObject', 'initialize', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'inspect', 'StringObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'length', 'StringObject', 'length', argc: 0, pass_env: false, pass_block: false, return_type: :size_t)
  gen.binding('String', 'ljust', 'StringObject', 'ljust', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'lstrip', 'StringObject', 'lstrip', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'lstrip!', 'StringObject', 'lstrip_in_place', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'match', 'StringObject', 'match', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'ord', 'StringObject', 'ord', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'reverse', 'StringObject', 'reverse', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'reverse!', 'StringObject', 'reverse_in_place', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'rstrip', 'StringObject', 'rstrip', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'rstrip!', 'StringObject', 'rstrip_in_place', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'size', 'StringObject', 'size', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'split', 'StringObject', 'split', argc: 0..2, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'start_with?', 'StringObject', 'start_with', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('String', 'strip', 'StringObject', 'strip', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'strip!', 'StringObject', 'strip_in_place', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'sub', 'StringObject', 'sub', argc: 1..2, pass_env: true, pass_block: true, return_type: :Object)
  gen.binding('String', 'succ', 'StringObject', 'successive', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'to_i', 'StringObject', 'to_i', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'to_s', 'StringObject', 'to_s', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('String', 'to_str', 'StringObject', 'to_str', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
  gen.binding('String', 'to_sym', 'StringObject', 'to_sym', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('String', 'upcase', 'StringObject', 'upcase', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

  gen.undefine_singleton_method('Symbol', 'new')
  gen.static_binding('Symbol', 'all_symbols', 'SymbolObject', 'all_symbols', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Symbol', '<=>', 'SymbolObject', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Symbol', 'empty?', 'SymbolObject', 'is_empty', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
  gen.binding('Symbol', 'id2name', 'SymbolObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Symbol', 'inspect', 'SymbolObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Symbol', 'intern', 'SymbolObject', 'to_sym', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Symbol', '[]', 'SymbolObject', 'ref', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Symbol', 'start_with?', 'SymbolObject', 'start_with', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('Symbol', 'succ', 'SymbolObject', 'succ', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Symbol', 'to_proc', 'SymbolObject', 'to_proc', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Symbol', 'to_s', 'SymbolObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Symbol', 'to_sym', 'SymbolObject', 'to_sym', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('Symbol', 'upcase', 'SymbolObject', 'upcase', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

  gen.undefine_singleton_method('TrueClass', 'new')
  gen.binding('TrueClass', '&', 'TrueObject', 'and_method', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('TrueClass', '|', 'TrueObject', 'or_method', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('TrueClass', '^', 'TrueObject', 'xor_method', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('TrueClass', 'inspect', 'TrueObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('TrueClass', 'to_s', 'TrueObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

  gen.undefine_singleton_method('UnboundMethod', 'new')
  gen.binding('UnboundMethod', '==', 'UnboundMethodObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
  gen.binding('UnboundMethod', 'arity', 'UnboundMethodObject', 'arity', argc: 0, pass_env: false, pass_block: false, return_type: :int)
  gen.binding('UnboundMethod', 'bind', 'UnboundMethodObject', 'bind', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('UnboundMethod', 'inspect', 'UnboundMethodObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('UnboundMethod', 'name', 'UnboundMethodObject', 'name', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
  gen.binding('UnboundMethod', 'owner', 'UnboundMethodObject', 'owner', argc: 0, pass_env: false, pass_block: false, return_type: :Object)

  gen.singleton_binding('main_obj', 'inspect', 'KernelModule', 'main_obj_inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
end

gen.init

puts
puts '}'
