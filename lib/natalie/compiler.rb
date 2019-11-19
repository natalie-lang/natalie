require 'tempfile'

module Natalie
  class Compiler
    BOILERPLATE = File.read(File.expand_path('main.c', __dir__))

    def initialize(ast = [])
      @ast = ast
      @var_num = 0
    end

    attr_accessor :ast

    def compile(out_path, shared: false)
      write_file
      cmd = "gcc -g -Wall -x c #{shared ? '-fPIC -shared' : ''} -I #{lib_path} -o #{out_path} #{@c_path} #{lib_path}/hashmap.c 2>&1"
      out = `#{cmd}`
      File.unlink(@c_path) unless ENV['DEBUG']
      if $? != 0
        $stderr.puts out
        raise 'There was an error compiling.'
      end
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
      @ast.each do |node|
        (t, d, e) = compile_expr(node)
        top << t
        decl << d
        body << "#{e};"
      end
      BOILERPLATE
        .sub('/*TOP*/', top.compact.join("\n"))
        .sub('/*DECL*/', decl.compact.join("\n"))
        .sub('/*BODY*/', body.compact.join("\n"))
    end

    def compile_expr(expr)
      if expr.is_a?(String)
        return [nil, nil, "env_get(env, #{expr.inspect})"]
      end
      case expr.first
      when :number
        var_name = next_var_name
        [nil, "NatObject *#{var_name} = nat_number(env, #{expr.last});", var_name]
      when :string
        var_name = next_var_name
        [nil, "NatObject *#{var_name} = nat_string(env, #{expr.last.inspect});", var_name]
      when :send
        (_, receiver, name, args) = expr
        top = []
        decl = []
        if args.any?
          args_name = next_var_name('args')
          decl << "NatObject **#{args_name} = calloc(#{args.size}, sizeof(NatObject));"
          args.each_with_index do |arg, i|
            (t, d, e) = compile_expr(arg);
            top << t; decl << d
            decl << "#{args_name}[#{i}] = #{e};"
          end
        else
          args_name = "NULL"
        end
        (t, d, e) = compile_expr(receiver)
        top << t; decl << d
        result_name = next_var_name('result')
        decl << "NatObject *#{result_name} = nat_send(env, #{e}, #{name.inspect}, #{args.size}, #{args_name});"
        [top, decl, result_name]
      else
        raise "unknown AST node: #{expr.inspect}"
      end
    end

    private

    def lib_path
      File.expand_path('.', __dir__)
    end

    def next_var_name(name = "var")
      @var_num += 1
      "#{name}#{@var_num}"
    end
  end
end
