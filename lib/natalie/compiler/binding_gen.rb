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
    binding(*args, **{ module_function: true, static: true, visibility: :private }.update(kwargs))
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
      puts "    #{binding.rb_class_as_c_variable}->#{binding.define_method_name}(env, #{binding.rb_method.inspect}_s, #{binding.name}, #{binding.arity}, #{binding.optimized ? 'true' : 'false'});"
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
      kwargs: [],
      visibility: :public,
      optimized: false
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
      @kwargs = kwargs
      @return_type = return_type
      @singleton = singleton
      @static = static
      @visibility = visibility
      @optimized = optimized
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
                :visibility,
                :optimized

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
Value #{name}(Env *env, Value self_value, Args args, Block *block) {
    #{pop_kwargs}
    #{argc_assertion}
    #{kwargs_assertion}
    #{type}self = #{as_type 'self_value'};
    auto return_value = self->#{cpp_method}(#{args_to_pass});
    #{return_code}
}\n
      FUNC
    end

    def write_static_function
      puts <<-FUNC
Value #{name}(Env *env, Value klass, Args args, Block *block) {
    #{pop_kwargs}
    #{argc_assertion}
    #{kwargs_assertion}
    auto return_value = #{cpp_class}::#{cpp_method}(#{args_to_pass});
    #{return_code}
}\n
      FUNC
    end

    def args_to_pass
      kwargs = @kwargs.map { |kw| "kwarg_#{kw}" }
      [env_arg, *args, *kwargs, block_arg, klass_arg].compact.join(', ')
    end

    def define_method_name
      if @module_function
        'define_method'
      elsif @singleton || @static
        'define_singleton_method'
      else
        'define_method'
      end
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
      rb_class.gsub('::', '')
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

    def pop_kwargs
      if @kwargs.any?
        "auto kwargs = args.pop_keyword_hash();\n" +
          @kwargs.map { |kw| "auto kwarg_#{kw} = kwargs ? kwargs->remove(env, #{kw.to_s.inspect}_s) : nullptr;" }.join("\n")
      end
    end

    def argc_assertion
      case argc
      when :any
        ''
      when Range
        if argc.end
          "args.ensure_argc_between(env, #{argc.begin}, #{argc.end});"
        else
          "args.ensure_argc_at_least(env, #{argc.begin});"
        end
      when Integer
        "args.ensure_argc_is(env, #{argc});"
      else
        raise "Unknown argc: #{argc.inspect}"
      end
    end

    def kwargs_assertion
      'env->ensure_no_extra_keywords(kwargs);' if @kwargs.any?
    end

    def env_arg
      'env' if pass_env
    end

    def args
      if max_argc
        (0...max_argc).map { |i| "args.at(#{i}, nullptr)" }
      else
        ['args']
      end
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
        underscored = cpp_class.sub(/Object|Object/, '').gsub(/([a-z])([A-Z])/, '\1_\2').gsub('::', '_').downcase
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
        'return new StringObject { return_value };'
      else
        raise "Unknown return type: #{return_type.inspect}"
      end
    end

    def max_argc
      case argc
      when :any
        nil
      when Range
        argc.end
      when Integer
        argc
      end
    end

    def generate_name
      @name = "#{rb_class_as_c_variable}_#{cpp_method}"
      if @module_function
        @name << '_module_function'
      else
        @name << '_singleton' if @singleton
        @name << '_static' if @static
        @name << "_#{@visibility}"
      end
      @name << '_binding'
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
gen.binding('Array', 'intersect?', 'ArrayObject', 'intersects', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
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

gen.binding('BasicObject', '__id__', 'Object', 'object_id', argc: 0, pass_env: false, pass_block: false, return_type: :int)
gen.binding('BasicObject', '__send__', 'Object', 'send', argc: 1.., pass_env: true, pass_block: true, return_type: :Object)
gen.binding('BasicObject', '!', 'Object', 'is_falsey', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('BasicObject', '==', 'Object', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('BasicObject', 'equal?', 'Object', 'equal', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('BasicObject', '!=', 'Object', 'neq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('BasicObject', 'initialize', 'Object', 'initialize', argc: 0, pass_env: true, pass_block: false, return_type: :Object, visibility: :private)
gen.binding('BasicObject', 'instance_eval', 'Object', 'instance_eval', argc: 0..1, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('BasicObject', 'method_missing', 'Object', 'method_missing', argc: :any, pass_env: true, pass_block: true, return_type: :Object, visibility: :private)

gen.undefine_instance_method('Class', 'module_function')
gen.binding('Class', 'initialize', 'ClassObject', 'initialize', argc: 0..1, pass_env: true, pass_block: true, return_type: :Object, visibility: :private)
gen.binding('Class', 'superclass', 'ClassObject', 'superclass', argc: 0, pass_env: true, pass_block: false, return_type: :NullableValue)
gen.binding('Class', 'singleton_class?', 'ClassObject', 'is_singleton', argc: 0, pass_env: false, pass_block: false, return_type: :bool)

gen.undefine_instance_method('Complex', 'new')
gen.binding('Complex', 'imaginary', 'ComplexObject', 'imaginary', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Complex', 'real', 'ComplexObject', 'real', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

gen.static_binding('Encoding', 'aliases', 'EncodingObject', 'aliases', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('Encoding', 'default_internal', 'EncodingObject', 'default_internal', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.static_binding('Encoding', 'default_internal=', 'EncodingObject', 'set_default_internal', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('Encoding', 'find', 'EncodingObject', 'find', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('Encoding', 'list', 'EncodingObject', 'list', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Encoding', 'ascii_compatible?', 'EncodingObject', 'is_ascii_compatible', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Encoding', 'inspect', 'EncodingObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Encoding', 'name', 'EncodingObject', 'name', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Encoding', 'names', 'EncodingObject', 'names', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

gen.undefine_singleton_method('EnumeratorArithmeticSequence', 'new')
gen.binding('Enumerator::ArithmeticSequence', '==', 'Enumerator::ArithmeticSequenceObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Enumerator::ArithmeticSequence', 'begin', 'Enumerator::ArithmeticSequenceObject', 'begin', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('Enumerator::ArithmeticSequence', 'each', 'Enumerator::ArithmeticSequenceObject', 'each', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('Enumerator::ArithmeticSequence', 'end', 'Enumerator::ArithmeticSequenceObject', 'end', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('Enumerator::ArithmeticSequence', 'exclude_end?', 'Enumerator::ArithmeticSequenceObject', 'exclude_end', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Enumerator::ArithmeticSequence', 'hash', 'Enumerator::ArithmeticSequenceObject', 'hash', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Enumerator::ArithmeticSequence', 'inspect', 'Enumerator::ArithmeticSequenceObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Enumerator::ArithmeticSequence', 'last', 'Enumerator::ArithmeticSequenceObject', 'last', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Enumerator::ArithmeticSequence', 'size', 'Enumerator::ArithmeticSequenceObject', 'size', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Enumerator::ArithmeticSequence', 'step', 'Enumerator::ArithmeticSequenceObject', 'step', argc: 0, pass_env: false, pass_block: false, return_type: :Object)


gen.singleton_binding('ENV', '[]', 'EnvObject', 'ref', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.singleton_binding('ENV', '[]=', 'EnvObject', 'refeq', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
gen.singleton_binding('ENV', 'assoc', 'EnvObject', 'assoc', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.singleton_binding('ENV', 'clear', 'EnvObject', 'clear', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.singleton_binding('ENV', 'delete', 'EnvObject', 'delete_key', argc: 1, pass_env: true, pass_block: true, return_type: :Object)
gen.singleton_binding('ENV', 'each', 'EnvObject', 'each', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.singleton_binding('ENV', 'empty?', 'EnvObject', 'is_empty', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.singleton_binding('ENV', 'fetch', 'EnvObject', 'fetch', argc: 1..2, pass_env: true, pass_block: true, return_type: :Object)
gen.singleton_binding('ENV', 'has_key?', 'EnvObject', 'has_key', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.singleton_binding('ENV', 'include?', 'EnvObject', 'has_key', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.singleton_binding('ENV', 'inspect', 'EnvObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.singleton_binding('ENV', 'key?', 'EnvObject', 'has_key', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.singleton_binding('ENV', 'keys', 'EnvObject', 'keys', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.singleton_binding('ENV', 'length', 'EnvObject', 'size', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.singleton_binding('ENV', 'member?', 'EnvObject', 'has_key', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.singleton_binding('ENV', 'rehash', 'EnvObject', 'rehash', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.singleton_binding('ENV', 'replace', 'EnvObject', 'replace', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.singleton_binding('ENV', 'size', 'EnvObject', 'size', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.singleton_binding('ENV', 'store', 'EnvObject', 'refeq', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
gen.singleton_binding('ENV', 'to_h', 'EnvObject', 'to_hash', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.singleton_binding('ENV', 'to_hash', 'EnvObject', 'to_hash', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.singleton_binding('ENV', 'to_s', 'EnvObject', 'to_s', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.singleton_binding('ENV', 'update', 'EnvObject', 'update', argc: :any, pass_env: true, pass_block: true, return_type: :Object)
gen.singleton_binding('ENV', 'merge!', 'EnvObject', 'update', argc: :any, pass_env: true, pass_block: true, return_type: :Object)

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

gen.static_binding('File', 'empty?', 'FileObject', 'is_zero', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'zero?', 'FileObject', 'is_zero', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'blockdev?', 'FileObject', 'is_blockdev', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'chardev?', 'FileObject', 'is_chardev', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'directory?', 'FileObject', 'is_directory', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'file?', 'FileObject', 'is_file', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'symlink?', 'FileObject', 'is_symlink', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'owned?', 'FileObject', 'is_owned', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'setuid?', 'FileObject', 'is_setuid', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'setgid?', 'FileObject', 'is_setgid', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'size', 'FileObject', 'size', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('File', 'size?', 'FileObject', 'is_size', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('File', 'sticky?', 'FileObject', 'is_sticky', argc: 1, pass_env: true, pass_block: false, return_type: :bool)

gen.static_binding('File', 'stat', 'FileObject', 'stat', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('File', 'lstat', 'FileObject', 'lstat', argc: 1, pass_env: true, pass_block: false, return_type: :Object)

gen.static_binding('File', 'socket?', 'FileObject', 'is_socket', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'readable?', 'FileObject', 'is_readable', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'writable?', 'FileObject', 'is_writable', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'executable?', 'FileObject', 'is_executable', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'readable_real?', 'FileObject', 'is_readable_real', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'writable_real?', 'FileObject', 'is_writable_real', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'executable_real?', 'FileObject', 'is_executable_real', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'pipe?', 'FileObject', 'is_pipe', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'exist?', 'FileObject', 'exist', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'identical?', 'FileObject', 'is_identical', argc: 2, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('File', 'link', 'FileObject', 'link', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('File', 'symlink', 'FileObject', 'symlink', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('File', 'mkfifo', 'FileObject', 'mkfifo', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('File', 'chmod', 'FileObject', 'chmod', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('File', 'world_readable?', 'FileObject', 'world_readable', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('File', 'world_writable?', 'FileObject', 'world_writable', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('File', 'expand_path', 'FileObject', 'expand_path', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('File', 'open', 'FileObject', 'open', argc: 1..2, pass_env: true, pass_block: true, return_type: :Object)
gen.static_binding('File', 'unlink', 'FileObject', 'unlink', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('File', 'delete', 'FileObject', 'unlink', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('File', 'ftype', 'FileObject', 'ftype', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('File', 'umask', 'FileObject', 'umask', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('File', 'realpath', 'FileObject', 'realpath', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
# NATFIXME: realdirpath is subtly different from realpath and should bind to a unique function
gen.static_binding('File', 'realdirpath', 'FileObject', 'realpath', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('File', 'chmod', 'FileObject', 'chmod', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('File', 'initialize', 'FileObject', 'initialize', argc: 1..2, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('File', 'path', 'FileObject', 'path', argc: 0, pass_env: false, pass_block: false, return_type: :String)
gen.binding('File', 'stat', 'FileObject', 'stat', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('File', 'lstat', 'FileObject', 'lstat', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

gen.static_binding('FileTest', 'blockdev?', 'FileObject', 'is_blockdev', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'chardev?', 'FileObject', 'is_chardev', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'directory?', 'FileObject', 'is_directory', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'empty?', 'FileObject', 'is_zero', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'executable?', 'FileObject', 'is_executable', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'executable_real?', 'FileObject', 'is_executable', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'exist?', 'FileObject', 'exist', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'file?', 'FileObject', 'is_file', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'identical?', 'FileObject', 'is_identical', argc: 2, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'owned?', 'FileObject', 'is_owned', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'pipe?', 'FileObject', 'is_pipe', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'readable?', 'FileObject', 'is_readable', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'readable_real?', 'FileObject', 'is_readable_real', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'setgid?', 'FileObject', 'is_setgid', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'setuid?', 'FileObject', 'is_setuid', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'size', 'FileObject', 'size', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('FileTest', 'size?', 'FileObject', 'is_size', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('FileTest', 'socket?', 'FileObject', 'is_socket', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'sticky?', 'FileObject', 'is_sticky', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'symlink?', 'FileObject', 'is_symlink', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'world_readable?', 'FileObject', 'world_readable', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'world_writable?', 'FileObject', 'world_writable', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'writable?', 'FileObject', 'is_writable', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'writable_real?', 'FileObject', 'is_writable_real', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('FileTest', 'zero?', 'FileObject', 'is_zero', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('File::Stat', 'initialize', 'FileStatObject', 'initialize', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'blockdev?', 'FileStatObject', 'is_blockdev', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('File::Stat', 'chardev?', 'FileStatObject', 'is_chardev', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('File::Stat', 'directory?', 'FileStatObject', 'is_directory', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('File::Stat', 'file?', 'FileStatObject', 'is_file', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('File::Stat', 'ftype', 'FileStatObject', 'ftype', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'pipe?', 'FileStatObject', 'is_pipe', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('File::Stat', 'owned?', 'FileStatObject', 'is_owned', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('File::Stat', 'setgid?', 'FileStatObject', 'is_setgid', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('File::Stat', 'setuid?', 'FileStatObject', 'is_setuid', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('File::Stat', 'size', 'FileStatObject', 'size', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'size?', 'FileStatObject', 'is_size', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'sticky?', 'FileStatObject', 'is_sticky', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('File::Stat', 'symlink?', 'FileStatObject', 'is_symlink', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('File::Stat', 'zero?', 'FileStatObject', 'is_zero', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('File::Stat', 'world_readable?', 'FileStatObject', 'world_readable', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'world_writable?', 'FileStatObject', 'world_writable', argc: 0, pass_env: false, pass_block: false, return_type: :Object)

gen.binding('File::Stat', 'atime', 'FileStatObject', 'atime', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'birthtime', 'FileStatObject', 'birthtime', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'ctime', 'FileStatObject', 'ctime', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'mtime', 'FileStatObject', 'mtime', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'comparison', 'FileStatObject', 'comparison', argc: 1, pass_env: true, pass_block: false, return_type: :Object)

gen.binding('File::Stat', 'blksize', 'FileStatObject', 'blksize', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'blocks', 'FileStatObject', 'blocks', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'dev', 'FileStatObject', 'dev', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'dev_major', 'FileStatObject', 'dev_major', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'dev_minor', 'FileStatObject', 'dev_minor', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'ino', 'FileStatObject', 'ino', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'nlink', 'FileStatObject', 'nlink', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'rdev', 'FileStatObject', 'rdev', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'rdev_major', 'FileStatObject', 'rdev_major', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'rdev_minor', 'FileStatObject', 'rdev_minor', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'uid', 'FileStatObject', 'uid', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'mode', 'FileStatObject', 'mode', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('File::Stat', 'gid', 'FileStatObject', 'gid', argc: 0, pass_env: false, pass_block: false, return_type: :Object)


gen.static_binding('Dir', 'delete', 'DirObject', 'rmdir', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('Dir', 'empty?', 'DirObject', 'is_empty', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('Dir', 'exist?', 'FileObject', 'is_directory', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.static_binding('Dir', 'home', 'DirObject', 'home', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('Dir', 'chdir', 'DirObject', 'chdir', argc: 0..1, pass_env: true, pass_block: true, return_type: :Object)
gen.static_binding('Dir', 'getwd', 'DirObject', 'pwd', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('Dir', 'mkdir', 'DirObject', 'mkdir', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('Dir', 'pwd', 'DirObject', 'pwd', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('Dir', 'rmdir', 'DirObject', 'rmdir', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('Dir', 'unlink', 'DirObject', 'rmdir', argc: 1, pass_env: true, pass_block: false, return_type: :Object)

gen.undefine_singleton_method('Float', 'new')
gen.binding('Float', '%', 'FloatObject', 'mod', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', '*', 'FloatObject', 'mul', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', '**', 'FloatObject', 'pow', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', '+', 'FloatObject', 'add', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', '+@', 'FloatObject', 'uplus', argc: 0, pass_env: false, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', '-', 'FloatObject', 'sub', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', '-@', 'FloatObject', 'uminus', argc: 0, pass_env: false, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', '/', 'FloatObject', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', '<', 'FloatObject', 'lt', argc: 1, pass_env: true, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Float', '<=', 'FloatObject', 'lte', argc: 1, pass_env: true, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Float', '<=>', 'FloatObject', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', '==', 'FloatObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Float', '===', 'FloatObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Float', '>', 'FloatObject', 'gt', argc: 1, pass_env: true, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Float', '>=', 'FloatObject', 'gte', argc: 1, pass_env: true, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Float', 'abs', 'FloatObject', 'abs', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'angle', 'FloatObject', 'arg', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'arg', 'FloatObject', 'arg', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'ceil', 'FloatObject', 'ceil', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'coerce', 'FloatObject', 'coerce', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: false)
gen.binding('Float', 'denominator', 'FloatObject', 'denominator', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'divmod', 'FloatObject', 'divmod', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'eql?', 'FloatObject', 'eql', argc: 1, pass_env: false, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Float', 'fdiv', 'FloatObject', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'finite?', 'FloatObject', 'is_finite', argc: 0, pass_env: false, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Float', 'floor', 'FloatObject', 'floor', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'infinite?', 'FloatObject', 'is_infinite', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'inspect', 'FloatObject', 'to_s', argc: 0, pass_env: false, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'magnitude', 'FloatObject', 'abs', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'modulo', 'FloatObject', 'mod', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'nan?', 'FloatObject', 'is_nan', argc: 0, pass_env: false, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Float', 'negative?', 'FloatObject', 'is_negative', argc: 0, pass_env: false, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Float', 'next_float', 'FloatObject', 'next_float', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'numerator', 'FloatObject', 'numerator', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'phase', 'FloatObject', 'arg', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'positive?', 'FloatObject', 'is_positive', argc: 0, pass_env: false, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Float', 'prev_float', 'FloatObject', 'prev_float', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'quo', 'FloatObject', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'round', 'FloatObject', 'round', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'to_f', 'FloatObject', 'to_f', argc: 0, pass_env: false, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'to_i', 'FloatObject', 'to_i', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'to_int', 'FloatObject', 'to_i', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'to_r', 'FloatObject', 'to_r', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'to_s', 'FloatObject', 'to_s', argc: 0, pass_env: false, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'truncate', 'FloatObject', 'truncate', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Float', 'zero?', 'FloatObject', 'is_zero', argc: 0, pass_env: false, pass_block: false, return_type: :bool, optimized: true)

gen.static_binding('GC', 'enable', 'GCModule', 'enable', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.static_binding('GC', 'disable', 'GCModule', 'disable', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.static_binding('GC', 'print_stats', 'GCModule', 'print_stats', argc: 0, pass_env: true, pass_block: false, return_type: :bool)

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
gen.static_binding('Integer', 'sqrt', 'IntegerObject', 'sqrt', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', '%', 'IntegerObject', 'mod', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', '&', 'IntegerObject', 'bitwise_and', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', '*', 'IntegerObject', 'mul', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', '**', 'IntegerObject', 'pow', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', '+', 'IntegerObject', 'add', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', '-', 'IntegerObject', 'sub', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', '-@', 'IntegerObject', 'negate', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', '~', 'IntegerObject', 'complement', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', '/', 'IntegerObject', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', '<=>', 'IntegerObject', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', '!=', 'Object', 'neq', argc: 1, pass_env: true, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Integer', '<', 'IntegerObject', 'lt', argc: 1, pass_env: true, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Integer', '<<', 'IntegerObject', 'left_shift', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', '<=', 'IntegerObject', 'lte', argc: 1, pass_env: true, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Integer', '>', 'IntegerObject', 'gt', argc: 1, pass_env: true, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Integer', '>>', 'IntegerObject', 'right_shift', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', '[]', 'IntegerObject', 'ref', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', '>=', 'IntegerObject', 'gte', argc: 1, pass_env: true, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Integer', '==', 'IntegerObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Integer', '===', 'IntegerObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Integer', '^', 'IntegerObject', 'bitwise_xor', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'abs', 'IntegerObject', 'abs', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'bit_length', 'IntegerObject', 'bit_length', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'chr', 'IntegerObject', 'chr', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'ceil', 'IntegerObject', 'ceil', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'coerce', 'IntegerObject', 'coerce', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: false)
gen.binding('Integer', 'denominator', 'IntegerObject', 'denominator', argc: 0, pass_env: false, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'even?', 'IntegerObject', 'is_even', argc: 0, pass_env: false, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Integer', 'floor', 'IntegerObject', 'floor', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'gcd', 'IntegerObject', 'gcd', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'inspect', 'IntegerObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'magnitude', 'IntegerObject', 'abs', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'modulo', 'IntegerObject', 'mod', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'next', 'IntegerObject', 'succ', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'numerator', 'IntegerObject', 'numerator', argc: 0, pass_env: false, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'odd?', 'IntegerObject', 'is_odd', argc: 0, pass_env: false, pass_block: false, return_type: :bool, optimized: true)
gen.binding('Integer', 'ord', 'IntegerObject', 'ord', argc: 0, pass_env: false, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'quo', 'IntegerObject', 'quo', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Integer', 'round', 'IntegerObject', 'round', argc: 0..1, kwargs: [:half], pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'size', 'IntegerObject', 'size', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'succ', 'IntegerObject', 'succ', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'pow', 'IntegerObject', 'powmod', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'pred', 'IntegerObject', 'pred', argc: 0, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'times', 'IntegerObject', 'times', argc: 0, pass_env: true, pass_block: true, return_type: :Object, optimized: false)
gen.binding('Integer', 'to_f', 'IntegerObject', 'to_f', argc: 0, pass_env: false, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'to_i', 'IntegerObject', 'to_i', argc: 0, pass_env: false, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'to_int', 'IntegerObject', 'to_i', argc: 0, pass_env: false, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'to_s', 'IntegerObject', 'to_s', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'truncate', 'IntegerObject', 'truncate', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', '|', 'IntegerObject', 'bitwise_or', argc: 1, pass_env: true, pass_block: false, return_type: :Object, optimized: true)
gen.binding('Integer', 'zero?', 'IntegerObject', 'is_zero', argc: 0, pass_env: false, pass_block: false, return_type: :bool, optimized: true)

gen.static_binding('IO', 'read', 'IoObject', 'read_file', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('IO', 'write', 'IoObject', 'write_file', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('IO', 'close', 'IoObject', 'close', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('IO', 'closed?', 'IoObject', 'is_closed', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('IO', 'fileno', 'IoObject', 'fileno', argc: 0, pass_env: false, pass_block: false, return_type: :int)
gen.binding('IO', 'initialize', 'IoObject', 'initialize', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('IO', 'print', 'IoObject', 'print', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('IO', 'gets', 'IoObject', 'gets', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('IO', 'puts', 'IoObject', 'puts', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('IO', 'read', 'IoObject', 'read', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('IO', 'seek', 'IoObject', 'seek', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('IO', 'write', 'IoObject', 'write', argc: 1.., pass_env: true, pass_block: false, return_type: :Object)

gen.module_function_binding('Kernel', 'Array', 'KernelModule', 'Array', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'abort', 'KernelModule', 'abort_method', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'at_exit', 'KernelModule', 'at_exit', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.module_function_binding('Kernel', 'binding', 'KernelModule', 'binding', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'block_given?', 'KernelModule', 'block_given', argc: 0, pass_env: true, pass_block: true, return_type: :bool)
gen.module_function_binding('Kernel', 'caller', 'KernelModule', 'caller', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'Complex', 'KernelModule', 'Complex', argc: 1..2, kwargs: [:exception], pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', '__dir__', 'KernelModule', 'cur_dir', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'exit', 'KernelModule', 'exit', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'exit!', 'KernelModule', 'exit_bang', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'Integer', 'KernelModule', 'Integer', argc: 1..2, kwargs: [:exception], pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'Float', 'KernelModule', 'Float', argc: 1, kwargs: [:exception], pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'fail', 'KernelModule', 'raise', argc: 0..2, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'fork', 'KernelModule', 'fork', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.module_function_binding('Kernel', 'gets', 'KernelModule', 'gets', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'get_usage', 'KernelModule', 'get_usage', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'Hash', 'KernelModule', 'Hash', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'lambda', 'KernelModule', 'lambda', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.module_function_binding('Kernel', '__method__', 'KernelModule', 'this_method', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'p', 'KernelModule', 'p', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'print', 'KernelModule', 'print', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'proc', 'KernelModule', 'proc', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.module_function_binding('Kernel', 'puts', 'KernelModule', 'puts', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'raise', 'KernelModule', 'raise', argc: 0..2, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'Rational', 'KernelModule', 'Rational', argc: 1..2, kwargs: [:exception], pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'sleep', 'KernelModule', 'sleep', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'spawn', 'KernelModule', 'spawn', argc: 1.., pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'srand', 'RandomObject', 'srand', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Kernel', 'String', 'KernelModule', 'String', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'class', 'KernelModule', 'klass_obj', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'clone', 'KernelModule', 'clone', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'define_singleton_method', 'KernelModule', 'define_singleton_method', argc: 1, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('Kernel', 'dup', 'KernelModule', 'dup', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', '===', 'KernelModule', 'equal', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Kernel', 'eql?', 'KernelModule', 'equal', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Kernel', 'extend', 'Object', 'extend', argc: 1.., pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'freeze', 'KernelModule', 'freeze_obj', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'frozen?', 'KernelModule', 'is_frozen', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Kernel', 'hash', 'KernelModule', 'hash', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'initialize_copy', 'KernelModule', 'initialize_copy', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'inspect', 'KernelModule', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'instance_variable_defined?', 'KernelModule', 'instance_variable_defined', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Kernel', 'instance_variable_get', 'KernelModule', 'instance_variable_get', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'instance_variable_set', 'KernelModule', 'instance_variable_set', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'instance_variables', 'KernelModule', 'instance_variables', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'is_a?', 'KernelModule', 'is_a', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Kernel', 'kind_of?', 'KernelModule', 'is_a', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Kernel', 'nil?', 'KernelModule', 'is_nil', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Kernel', 'loop', 'KernelModule', 'loop', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('Kernel', 'method', 'KernelModule', 'method', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'methods', 'KernelModule', 'methods', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'public_methods', 'KernelModule', 'methods', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'public_send', 'Object', 'public_send', argc: 1.., pass_env: true, pass_block: true, return_type: :Object)
gen.binding('Kernel', 'object_id', 'Object', 'object_id', argc: 0, pass_env: false, pass_block: false, return_type: :int)
gen.binding('Kernel', 'remove_instance_variable', 'KernelModule', 'remove_instance_variable', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'respond_to?', 'KernelModule', 'respond_to_method', argc: 1..2, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Kernel', 'send', 'Object', 'send', argc: 1.., pass_env: true, pass_block: true, return_type: :Object)
gen.binding('Kernel', 'singleton_class', 'KernelModule', 'singleton_class_obj', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Kernel', 'tap', 'KernelModule', 'tap', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('Kernel', 'to_s', 'KernelModule', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

gen.undefine_singleton_method('MatchData', 'new')
gen.undefine_singleton_method('MatchData', 'allocate')
gen.binding('MatchData', 'captures', 'MatchDataObject', 'captures', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('MatchData', 'size', 'MatchDataObject', 'size', argc: 0, pass_env: false, pass_block: false, return_type: :size_t)
gen.binding('MatchData', 'length', 'MatchDataObject', 'size', argc: 0, pass_env: false, pass_block: false, return_type: :size_t)
gen.binding('MatchData', 'offset', 'MatchDataObject', 'offset', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('MatchData', 'to_a', 'MatchDataObject', 'to_a', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
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
gen.binding('Module', 'const_get', 'ModuleObject', 'const_get', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Module', 'const_set', 'ModuleObject', 'const_set', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Module', 'constants', 'ModuleObject', 'constants', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
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

gen.singleton_binding('NatalieParser', 'parse', 'ParserObject', 'parse', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
gen.singleton_binding('NatalieParser', 'tokens', 'ParserObject', 'tokens', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)

gen.undefine_singleton_method('NilClass', 'new')
gen.binding('NilClass', '&', 'NilObject', 'and_method', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('NilClass', '|', 'NilObject', 'or_method', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('NilClass', '^', 'NilObject', 'or_method', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('NilClass', '=~', 'NilObject', 'eqtilde', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('NilClass', 'inspect', 'NilObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('NilClass', 'rationalize', 'NilObject', 'rationalize', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('NilClass', 'to_a', 'NilObject', 'to_a', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('NilClass', 'to_c', 'NilObject', 'to_c', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('NilClass', 'to_f', 'NilObject', 'to_f', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('NilClass', 'to_h', 'NilObject', 'to_h', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('NilClass', 'to_i', 'NilObject', 'to_i', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('NilClass', 'to_r', 'NilObject', 'to_r', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('NilClass', 'to_s', 'NilObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

gen.binding('Object', 'nil?', 'Object', 'is_nil', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Object', 'itself', 'Object', 'itself', argc: 0, pass_env: false, pass_block: false, return_type: :Object)

gen.binding('Proc', '[]', 'ProcObject', 'call', argc: :any, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('Proc', 'initialize', 'ProcObject', 'initialize', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('Proc', 'arity', 'ProcObject', 'arity', argc: 0, pass_env: false, pass_block: false, return_type: :int)
gen.binding('Proc', 'call', 'ProcObject', 'call', argc: :any, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('Proc', 'lambda?', 'ProcObject', 'is_lambda', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Proc', 'to_proc', 'ProcObject', 'to_proc', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

gen.module_function_binding('Process', 'abort', 'KernelModule', 'abort_method', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process', 'egid', 'ProcessModule', 'egid', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process', 'egid=', 'ProcessModule', 'setegid', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process', 'euid', 'ProcessModule', 'euid', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process', 'euid=', 'ProcessModule', 'seteuid', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process', 'exit', 'KernelModule', 'exit', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process', 'exit!', 'KernelModule', 'exit_bang', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process', 'gid', 'ProcessModule', 'gid', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process', 'gid=', 'ProcessModule', 'setgid', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process', 'pid', 'ProcessModule', 'pid', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process', 'ppid', 'ProcessModule', 'ppid', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process', 'uid', 'ProcessModule', 'uid', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process', 'uid=', 'ProcessModule', 'setuid', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process::GID', 'eid', 'ProcessModule', 'egid', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process::GID', 'rid', 'ProcessModule', 'gid', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process::Sys', 'getegid', 'ProcessModule', 'egid', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process::Sys', 'geteuid', 'ProcessModule', 'euid', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process::Sys', 'getgid', 'ProcessModule', 'gid', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process::Sys', 'getuid', 'ProcessModule', 'uid', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process::UID', 'eid', 'ProcessModule', 'euid', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.module_function_binding('Process::UID', 'rid', 'ProcessModule', 'uid', argc: 0, pass_env: true, pass_block: false, return_type: :Object)


gen.static_binding('Random', 'new_seed', 'RandomObject', 'new_seed', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('Random', 'srand', 'RandomObject', 'srand', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Random', 'initialize', 'RandomObject', 'initialize', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object, visibility: :private)
gen.binding('Random', 'rand', 'RandomObject', 'rand', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Random', 'seed', 'RandomObject', 'seed', argc: 0, pass_env: false, pass_block: false, return_type: :Object)

gen.binding('Range', '%', 'RangeObject', 'step', argc: 0..1, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('Range', '==', 'RangeObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Range', 'begin', 'RangeObject', 'begin', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('Range', 'bsearch', 'RangeObject', 'bsearch', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('Range', 'end', 'RangeObject', 'end', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('Range', 'each', 'RangeObject', 'each', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('Range', 'eql?', 'RangeObject', 'eql', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Range', 'exclude_end?', 'RangeObject', 'exclude_end', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Range', 'first', 'RangeObject', 'first', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Range', 'include?', 'RangeObject', 'include', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Range', 'initialize', 'RangeObject', 'initialize', argc: 2..3, pass_env: true, pass_block: false, return_type: :Object, visibility: :private)
gen.binding('Range', 'inspect', 'RangeObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Range', 'last', 'RangeObject', 'last', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Range', 'member?', 'RangeObject', 'include', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Range', 'step', 'RangeObject', 'step', argc: 0..1, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('Range', 'to_a', 'RangeObject', 'to_a', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Range', 'to_s', 'RangeObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

gen.undefine_singleton_method('Rational', 'new')
gen.binding('Rational', '*', 'RationalObject', 'mul', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Rational', '**', 'RationalObject', 'pow', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Rational', '+', 'RationalObject', 'add', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Rational', '-', 'RationalObject', 'sub', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Rational', '/', 'RationalObject', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Rational', '<=>', 'RationalObject', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Rational', '==', 'RationalObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Rational', '===', 'RationalObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Rational', 'coerce', 'RationalObject', 'coerce', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Rational', 'denominator', 'RationalObject', 'denominator', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Rational', 'floor', 'RationalObject', 'floor', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Rational', 'inspect', 'RationalObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Rational', 'marshal_dump', 'RationalObject', 'marshal_dump', argc: 0, pass_env: true, pass_block: false, return_type: :Object, visibility: :private)
gen.binding('Rational', 'numerator', 'RationalObject', 'numerator', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Rational', 'quo', 'RationalObject', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Rational', 'to_f', 'RationalObject', 'to_f', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Rational', 'to_i', 'RationalObject', 'to_i', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Rational', 'to_r', 'RationalObject', 'to_r', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('Rational', 'to_s', 'RationalObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Rational', 'zero?', 'RationalObject', 'is_zero', argc: 0, pass_env: false, pass_block: false, return_type: :bool)

gen.static_binding('Regexp', 'compile', 'RegexpObject', 'compile', argc: 1..2, pass_env: true, pass_block: false, pass_klass: true, return_type: :Object)
gen.static_binding('Regexp', 'last_match', 'RegexpObject', 'last_match', argc: 0..1, pass_env: true, pass_block: false, pass_klass: false, return_type: :Object)
gen.binding('Regexp', '==', 'RegexpObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Regexp', 'eql?', 'RegexpObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Regexp', '===', 'RegexpObject', 'eqeqeq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Regexp', '=~', 'RegexpObject', 'eqtilde', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Regexp', '~', 'RegexpObject', 'tilde', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Regexp', 'casefold?', 'RegexpObject', 'casefold', argc: 0, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Regexp', 'encoding', 'RegexpObject', 'encoding', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
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
gen.binding('String', '-@', 'StringObject', 'uminus', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', '<<', 'StringObject', 'ltlt', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', '<=>', 'StringObject', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', '==', 'StringObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('String', '===', 'StringObject', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('String', '=~', 'StringObject', 'eqtilde', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', '[]', 'StringObject', 'ref', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', '[]=', 'StringObject', 'refeq', argc: 1..3, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'ascii_only?', 'StringObject', 'ascii_only', argc: 0, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('String', 'b', 'StringObject', 'b', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'bytes', 'StringObject', 'bytes', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('String', 'byteslice', 'StringObject', 'byteslice', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'bytesize', 'StringObject', 'bytesize', argc: 0, pass_env: false, pass_block: false, return_type: :size_t)
gen.binding('String', 'center', 'StringObject', 'center', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'chars', 'StringObject', 'chars', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('String', 'chomp', 'StringObject', 'chomp', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'chomp!', 'StringObject', 'chomp_in_place', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'chop', 'StringObject', 'chop', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'chop!', 'StringObject', 'chop_in_place', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'chr', 'StringObject', 'chr', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'clear', 'StringObject', 'clear', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'clone', 'StringObject', 'clone', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'codepoints', 'StringObject', 'codepoints', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('String', 'concat', 'StringObject', 'concat', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'downcase', 'StringObject', 'downcase', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'each_byte', 'StringObject', 'each_byte', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('String', 'each_char', 'StringObject', 'each_char', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('String', 'each_codepoint', 'StringObject', 'each_codepoint', argc: 0, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('String', 'empty?', 'StringObject', 'is_empty', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('String', 'encode', 'StringObject', 'encode', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'encoding', 'StringObject', 'encoding', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('String', 'end_with?', 'StringObject', 'end_with', argc: :any, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('String', 'eql?', 'StringObject', 'eql', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('String', 'force_encoding', 'StringObject', 'force_encoding', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'getbyte', 'StringObject', 'getbyte', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'gsub', 'StringObject', 'gsub', argc: 1..2, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('String', 'hex', 'StringObject', 'hex', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'include?', 'StringObject', 'include', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('String', 'index', 'StringObject', 'index', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'initialize', 'StringObject', 'initialize', argc: 0..1, kwargs: [:encoding, :capacity], pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'initialize_copy', 'StringObject', 'initialize_copy', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'insert', 'StringObject', 'insert', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'inspect', 'StringObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'intern', 'StringObject', 'to_sym', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'length', 'StringObject', 'size', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'lines', 'StringObject', 'lines', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'ljust', 'StringObject', 'ljust', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'lstrip', 'StringObject', 'lstrip', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'lstrip!', 'StringObject', 'lstrip_in_place', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'match', 'StringObject', 'match', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'next', 'StringObject', 'successive', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'next!', 'StringObject', 'successive_in_place', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'prepend', 'StringObject', 'prepend', argc: :any, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'ord', 'StringObject', 'ord', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'replace', 'StringObject', 'initialize_copy', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'reverse', 'StringObject', 'reverse', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'reverse!', 'StringObject', 'reverse_in_place', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'rjust', 'StringObject', 'rjust', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'rstrip', 'StringObject', 'rstrip', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'rstrip!', 'StringObject', 'rstrip_in_place', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'setbyte', 'StringObject', 'setbyte', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'size', 'StringObject', 'size', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'slice!', 'StringObject', 'slice_in_place', argc: 1..2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'split', 'StringObject', 'split', argc: 0..2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'start_with?', 'StringObject', 'start_with', argc: :any, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('String', 'strip', 'StringObject', 'strip', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'strip!', 'StringObject', 'strip_in_place', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'sub', 'StringObject', 'sub', argc: 1..2, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('String', 'succ', 'StringObject', 'successive', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'succ!', 'StringObject', 'successive_in_place', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'to_f', 'StringObject', 'to_f', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'to_i', 'StringObject', 'to_i', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'to_s', 'StringObject', 'to_s', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('String', 'to_str', 'StringObject', 'to_str', argc: 0, pass_env: false, pass_block: false, return_type: :Object)
gen.binding('String', 'to_sym', 'StringObject', 'to_sym', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'tr', 'StringObject', 'tr', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'tr!', 'StringObject', 'tr_in_place', argc: 2, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('String', 'try_convert', 'StringObject', 'try_convert', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'unpack', 'StringObject', 'unpack', argc: 1, kwargs: [:offset], pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'unpack1', 'StringObject', 'unpack1', argc: 1, kwargs: [:offset], pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'upcase', 'StringObject', 'upcase', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'upto', 'StringObject', 'upto', argc: 1..2, pass_env: true, pass_block: true, return_type: :Object)
gen.binding('String', 'valid_encoding?', 'StringObject', 'valid_encoding', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('String', 'delete_prefix', 'StringObject', 'delete_prefix', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'delete_prefix!', 'StringObject', 'delete_prefix_in_place', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'delete_suffix', 'StringObject', 'delete_suffix', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'delete_suffix!', 'StringObject', 'delete_suffix_in_place', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'partition', 'StringObject', 'partition', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('String', 'sum', 'StringObject', 'sum', argc: 0..1, pass_env: true, pass_block: false, return_type: :Object)

gen.undefine_singleton_method('Symbol', 'new')
gen.static_binding('Symbol', 'all_symbols', 'SymbolObject', 'all_symbols', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Symbol', '<=>', 'SymbolObject', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Symbol', '=~', 'SymbolObject', 'eqtilde', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Symbol', 'empty?', 'SymbolObject', 'is_empty', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Symbol', 'id2name', 'SymbolObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Symbol', 'inspect', 'SymbolObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Symbol', 'intern', 'SymbolObject', 'to_sym', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Symbol', 'length', 'SymbolObject', 'length', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Symbol', 'name', 'SymbolObject', 'name', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Symbol', '[]', 'SymbolObject', 'ref', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Symbol', 'size', 'SymbolObject', 'length', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Symbol', 'start_with?', 'SymbolObject', 'start_with', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Symbol', 'succ', 'SymbolObject', 'succ', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Symbol', 'to_proc', 'SymbolObject', 'to_proc', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Symbol', 'to_s', 'SymbolObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Symbol', 'to_sym', 'SymbolObject', 'to_sym', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Symbol', 'upcase', 'SymbolObject', 'upcase', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

gen.static_binding('Time', 'at', 'TimeObject', 'at', argc: 1..3, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('Time', 'local', 'TimeObject', 'local', argc: 1..7, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('Time', 'new', 'TimeObject', 'create', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('Time', 'now', 'TimeObject', 'now', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.static_binding('Time', 'utc', 'TimeObject', 'utc', argc: 1..7, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', '+', 'TimeObject', 'add', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', '-', 'TimeObject', 'minus', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', '<=>', 'TimeObject', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'asctime', 'TimeObject', 'asctime', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'eql?', 'TimeObject', 'eql', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Time', 'hour', 'TimeObject', 'hour', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'inspect', 'TimeObject', 'inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'mday', 'TimeObject', 'mday', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'min', 'TimeObject', 'min', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'month', 'TimeObject', 'month', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'nsec', 'TimeObject', 'nsec', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'sec', 'TimeObject', 'sec', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'strftime', 'TimeObject', 'strftime', argc: 1, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'subsec', 'TimeObject', 'subsec', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'to_f', 'TimeObject', 'to_f', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'to_i', 'TimeObject', 'to_i', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'to_r', 'TimeObject', 'to_r', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'to_s', 'TimeObject', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'usec', 'TimeObject', 'usec', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'utc?', 'TimeObject', 'is_utc', argc: 0, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Time', 'utc_offset', 'TimeObject', 'utc_offset', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'wday', 'TimeObject', 'wday', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'yday', 'TimeObject', 'yday', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.binding('Time', 'year', 'TimeObject', 'year', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

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

gen.singleton_binding('main_obj', 'define_method', 'Object', 'main_obj_define_method', argc: 1..2, pass_env: true, pass_block: true, return_type: :Object)
gen.singleton_binding('main_obj', 'inspect', 'KernelModule', 'main_obj_inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)
gen.singleton_binding('main_obj', 'to_s', 'KernelModule', 'main_obj_inspect', argc: 0, pass_env: true, pass_block: false, return_type: :Object)

gen.init

puts
puts '}'
