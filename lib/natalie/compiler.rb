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
        decl << "env_set(env, #{name.inspect}, #{var_name});"
        [t, decl, var_name]
      when :class
        (_, name, superclass, body) = expr
        func_name = next_var_name('class_body')
        top = []
        func = []
        func << "NatObject* #{func_name}(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {"
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
        func << '}'
        var_name = next_var_name('class')
        decl = []
        superclass ||= 'Object'
        decl << "NatObject *#{var_name} = nat_subclass(env_get(env, #{superclass.inspect}), #{name.inspect});"
        decl << "env_set(env, #{name.inspect}, #{var_name});"
        result_name = next_var_name('class_body_result')
        decl << "NatObject *#{result_name} = #{func_name}(env, #{var_name}, 0, NULL, NULL);"
        [top + func, decl, result_name]
      when :module
        (_, name, body) = expr
        func_name = next_var_name('module_body')
        top = []
        func = []
        func << "NatObject* #{func_name}(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {"
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
        func << '}'
        var_name = next_var_name('module')
        decl = []
        decl << "NatObject *#{var_name} = nat_module(env, #{name.inspect});"
        decl << "env_set(env, #{name.inspect}, #{var_name});"
        result_name = next_var_name('module_body_result')
        decl << "NatObject *#{result_name} = #{func_name}(env, #{var_name}, 0, NULL, NULL);"
        [top + func, decl, result_name]
      when :def
        (_, name, args, kwargs, body) = expr
        func_name = next_var_name('func')
        top = []
        func = []
        func << "NatObject* #{func_name}(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {"
        func << "if (argc != #{args.size}) abort();" # FIXME
        # TODO: do something with kwargs
        func << "env = build_env(env);"
        func << "env_set(env, \"__method__\", nat_string(env, #{name.inspect}));"
        args.each_with_index do |arg, i|
          func << "env_set(env, #{arg.inspect}, args[#{i}]);"
        end
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
        func << "}"
        method_name = next_var_name('method')
        decl = []
        decl << "nat_define_method(self, #{name.inspect}, #{func_name});"
        decl << "NatObject *#{method_name} = nat_string(env, #{name.inspect});"
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
        (_, receiver, name, args) = expr
        top = []
        decl = []
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
        result_name = next_var_name('result')
        if receiver.nil? && name == 'super'
          decl << "NatObject *#{result_name} = nat_call_method_on_class(env, self->class->superclass, self->class->superclass, env_get(env, \"__method__\")->str, self, #{args.size}, #{args_name});"
        else
          (t, d, e) = compile_expr(receiver || 'self')
          top << t; decl << d
          if receiver.nil?
            decl << "NatObject *#{result_name} = nat_lookup_or_send(env, #{e}, #{name.inspect}, #{args.size}, #{args_name});"
          else
            decl << "NatObject *#{result_name} = nat_send(env, #{e}, #{name.inspect}, #{args.size}, #{args_name});"
          end
        end
        [top, decl, result_name]
      else
        raise "unknown AST node: #{expr.inspect}"
      end
    end

    def next_var_name(name = "var")
      @var_num += 1
      "#{name}#{@var_num}"
    end
  end
end
