require 'tempfile'

module Natalie
  class Compiler
    SRC_PATH = File.expand_path('../../src', __dir__)
    MAIN = File.read(File.join(SRC_PATH, 'main.c'))

    def initialize(ast = [])
      @ast = ast
      @var_num = 0
    end

    attr_accessor :ast

    def compile(out_path, shared: false)
      write_file
      cmd = "gcc -g -Wall -x c #{shared ? '-fPIC -shared' : ''} -I #{SRC_PATH} -o #{out_path} #{@c_path} #{c_files_to_compile.join(' ')} 2>&1"
      out = `#{cmd}`
      File.unlink(@c_path) unless ENV['DEBUG']
      $stderr.puts out if ENV['DEBUG'] || $? != 0
      raise 'There was an error compiling.' if $? != 0
    end

    def c_files_to_compile
      Dir[File.join(SRC_PATH, '*.c')].grep_v(/main\.c$/)
    end

    def write_file
      c = to_c
      puts c if ENV['DEBUG']
      temp_c = Tempfile.create('natalie.c')
      temp_c.write(c)
      temp_c.close
      @c_path = temp_c.path
    end

    def to_c
      top = []
      decl = []
      body = []
      @ast.each_with_index do |node, i|
        (t, d, e) = compile_expr(node)
        top << t
        decl << d
        if i == @ast.size-1 && e
          body << "return #{e};"
        elsif e
          body << "UNUSED(#{e});"
        end
      end
      out = MAIN
        .sub('/*TOP*/', top.compact.join("\n"))
        .sub('/*DECL*/', decl.compact.join("\n"))
        .sub('/*BODY*/', body.compact.join("\n"))
      reindent(out)
    end

    private

    def reindent(code)
      out = []
      indent = 0
      code.split("\n").each do |line|
        indent -= 4 if line =~ /^\s*\}/
        out << line.sub(/^\s*/, ' ' * indent)
        indent += 4 if line.end_with?('{')
      end
      out.join("\n")
    end

    def compile_expr(expr)
      if expr == 'self'
        return [nil, nil, 'self']
      end
      if expr.is_a?(String)
        return [nil, nil, "env_get(env, #{expr.inspect})"]
      end
      case expr.first
      when :assign
        var_name = next_var_name
        (_, name, value) = expr
        (t, d, e) = compile_expr(value)
        decl = [d]
        decl << "NatObject *#{var_name} = #{e};"
        if name.is_a?(Array) && name.first == :ivar
          decl << "ivar_set(env, self, #{name.last.inspect}, #{var_name});"
        else
          decl << "env_set(env, #{name.inspect}, #{var_name});"
        end
        [t, decl, var_name]
      when :class
        (_, name, superclass, body) = expr
        func_name = next_var_name('class_body')
        top = []
        func = []
        func << "NatObject* #{func_name}(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {"
        (t, f) = compile_func_body(body)
        top << t
        func << f
        func << '}'
        var_name = next_var_name('class')
        decl = []
        superclass ||= 'Object'
        decl << "NatObject *#{var_name} = env_get(env, #{name.inspect});"
        decl << "if (!#{var_name}) {"
        decl << "#{var_name} = nat_subclass(env, env_get(env, #{superclass.inspect}), #{name.inspect});"
        decl << "env_set(env, #{name.inspect}, #{var_name});"
        decl << '}'
        result_name = next_var_name('class_body_result')
        decl << "NatObject *#{result_name} = #{func_name}(env, #{var_name}, 0, NULL, NULL, NULL);"
        [top + func, decl, result_name]
      when :module
        (_, name, body) = expr
        func_name = next_var_name('module_body')
        top = []
        func = []
        func << "NatObject* #{func_name}(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {"
        (t, f) = compile_func_body(body)
        top << t
        func << f
        func << '}'
        var_name = next_var_name('module')
        decl = []
        decl << "NatObject *#{var_name} = env_get(env, #{name.inspect});"
        decl << "if (!#{var_name}) {"
        decl << "#{var_name} = nat_module(env, #{name.inspect});"
        decl << "env_set(env, #{name.inspect}, #{var_name});"
        decl << '}'
        result_name = next_var_name('module_body_result')
        decl << "NatObject *#{result_name} = #{func_name}(env, #{var_name}, 0, NULL, NULL, NULL);"
        [top + func, decl, result_name]
      when :def
        (_, owner, name, args, kwargs, body) = expr
        func_name = next_var_name('func')
        top = []
        func = []
        func << "NatObject* #{func_name}(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {"
        func << "if (argc != #{args.size}) abort();" # FIXME
        # TODO: do something with kwargs
        func << "env = build_env(env);"
        func << "env_set(env, \"__method__\", nat_string(env, #{name.inspect}));"
        args.each_with_index do |arg, i|
          func << "env_set(env, #{arg.inspect}, args[#{i}]);"
        end
        (t, f) = compile_func_body(body)
        top << t; func << f
        func << '}'
        method_name = next_var_name('method')
        decl = []
        if owner
          owner_ref = owner == 'self' ? owner : "env_get(env, #{owner.inspect})"
          decl << "nat_define_singleton_method(#{owner_ref}, #{name.inspect}, #{func_name});"
        else
          decl << "nat_define_method(self, #{name.inspect}, #{func_name});"
        end
        decl << "NatObject *#{method_name} = nat_symbol(env, #{name.inspect});"
        [top + func, decl, method_name]
      when :integer
        var_name = next_var_name('integer')
        [nil, "NatObject *#{var_name} = nat_integer(env, #{expr.last});", var_name]
      when :string
        var_name = next_var_name('string')
        [nil, "NatObject *#{var_name} = nat_string(env, #{expr.last.inspect});", var_name]
      when :symbol
        var_name = next_var_name('symbol')
        [nil, "NatObject *#{var_name} = nat_symbol(env, #{expr.last.inspect});", var_name]
      when :array
        var_name = next_var_name('array')
        top = []
        decl = []
        decl << "NatObject *#{var_name} = nat_array(env);"
        expr.last.each do |node|
          (t, d, e) = compile_expr(node)
          top << t; decl << d
          decl << "nat_array_push(#{var_name}, #{e});"
        end
        [top, decl, var_name]
      when :send
        (_, receiver, name, args, block) = expr
        top = []
        decl = []
        # args
        if args.empty?
          args_name = "NULL"
        elsif args.size == 1
          (t, d, e) = compile_expr(args.first)
          top << t; decl << d
          args_name = "&#{e}"
        elsif args.any?
          args_name = next_var_name('args')
          decl << "NatObject **#{args_name} = calloc(#{args.size}, sizeof(NatObject));"
          args.each_with_index do |arg, i|
            (t, d, e) = compile_expr(arg);
            top << t; decl << d
            decl << "#{args_name}[#{i}] = #{e};"
          end
        end
        # block
        block_name = 'NULL'
        if block
          (_, block_args, block_body) = block
          block_func_name = next_var_name('block_func')
          func = []
          func << "NatObject* #{block_func_name}(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {"
          func << "env = build_env(env);"
          func << "env->block = TRUE;"
          block_args.each_with_index do |arg, i|
            func << "if (#{i} < argc) {"
            func << "env_set(env, #{arg.inspect}, args[#{i}]);"
            func << "} else {"
            func << "env_set(env, #{arg.inspect}, env_get(env, \"nil\"));"
            func << '}'
          end
          (t, f) = compile_func_body(block_body)
          top << t
          func << f
          func << '}'
          top << func
          block_name = next_var_name('block')
          decl << "NatBlock *#{block_name} = nat_block(env, #{block_func_name});"
        end
        # call
        result_name = next_var_name('result')
        if receiver.nil? && name == 'super'
          decl << "NatObject *#{result_name} = nat_call_method_on_class(env, self->class->superclass, self->class->superclass, env_get(env, \"__method__\")->str, self, #{args.size}, #{args_name}, NULL, #{block_name});"
        elsif receiver.nil? && name == 'yield'
          decl << "NatObject *#{result_name} = block->fn(block->env, self, #{args.size}, #{args_name}, NULL, NULL);"
        else
          (t, d, e) = compile_expr(receiver || 'self')
          top << t; decl << d
          if receiver.nil?
            decl << "NatObject *#{result_name} = nat_lookup_or_send(env, #{e}, #{name.inspect}, #{args.size}, #{args_name}, #{block_name});"
          else
            decl << "NatObject *#{result_name} = nat_send(env, #{e}, #{name.inspect}, #{args.size}, #{args_name}, #{block_name});"
          end
        end
        [top, decl, result_name]
      when :if
        top = []
        decl = []
        result_name = next_var_name('if_result')
        c_if = ["NatObject *#{result_name};"]
        expr[1..-1].each_slice(2).each_with_index do |(condition, body), index|
          func_name = next_var_name('if_body_func')
          func = []
          func << "NatObject* #{func_name}(NatEnv *env, NatObject *self) {"
          func << "env = build_env(env);"
          (t, f) = compile_func_body(body)
          top << t
          func << f
          func << '}'
          top << func
          if index == 0
            (t, d, c) = compile_expr(condition)
            top << t; decl << d
            c_if << "if (nat_truthy(#{c})) #{result_name} = #{func_name}(env, self);"
          elsif condition != :else
            (t, d, c) = compile_expr(condition)
            top << t; decl << d
            c_if << "else if (nat_truthy(#{c})) #{result_name} = #{func_name}(env, self);"
          else
            c_if << "else #{result_name} = #{func_name}(env, self);"
          end
        end
        [top, decl + c_if, result_name]
      when :while
        (_, condition, body) = expr
        top = []
        decl = []
        func_name = next_var_name('while_body_func')
        func = []
        func << "NatObject* #{func_name}(NatEnv *env, NatObject *self) {"
        (t, f) = compile_func_body(body)
        top << t
        func << f
        func << '}'
        top << func
        decl << "while (TRUE) {"
        (t, d, c) = compile_expr(condition)
        top << t; decl << d
        decl << "if (!nat_truthy(#{c})) break;"
        decl << "#{func_name}(env, self);"
        decl << '}'
        [top, decl, "env_get(env, \"nil\")"]
      when :ivar
        name = expr.last
        [nil, nil, "ivar_get(env, self, #{name.inspect})"]
      else
        raise "unknown AST node: #{expr.inspect}"
      end
    end

    def next_var_name(name = "var")
      @var_num += 1
      "#{name}#{@var_num}"
    end

    def compile_func_body(body)
      top = []
      func = []
      if body.any?
        body.each_with_index do |node, i|
          (t, d, e) = compile_expr(node)
          top << t
          func << d
          if i == body.size-1 && e
            func << "return #{e};"
          elsif e
            func << "UNUSED(#{e});"
          end
        end
      else
        func << "return env_get(env, \"nil\");"
      end
      [top, func]
    end
  end
end
