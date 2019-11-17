require 'tempfile'

module Natalie
  class Compiler
    BOILERPLATE = File.read(File.expand_path('boilerplate.c', __dir__))

    def initialize(ast = [])
      @ast = ast
      @var_num = 0
    end

    attr_accessor :ast

    def compile(out_path)
      write_file
      out = `gcc -g -x c -I #{lib_path} -o #{out_path} #{@c_path} #{lib_path}/hashmap.c 2>&1`
      File.unlink(@c_path) unless ENV['DEBUG']
      if $? != 0
        puts "There was an error compiling:\n#{out}"
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
      exprs = @ast.map { |e| compile_expr(e) }.join("\n")
      BOILERPLATE.sub('/*MAIN*/', exprs)
    end

    def compile_expr(expr)
      case expr.first
      when :number
        "NatObject *#{next_var_name} = nat_number(env, #{expr.last});"
      when :string
        "NatObject *#{next_var_name} = nat_string(env, #{expr.last.inspect});"
      else
        raise "unknown AST node: #{expr.inspect}"
      end
    end

    private

    def lib_path
      File.expand_path('.', __dir__)
    end

    def next_var_name
      @var_num += 1
      "var#{@var_num}"
    end
  end
end
