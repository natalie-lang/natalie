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
      puts @c_path if ENV['DEBUG']
      out = `gcc -g -x c -o #{out_path} #{@c_path} 2>&1`
      if $? == 0
        File.unlink(@c_path) unless ENV['DEBUG']
      else
        puts "There was an error compiling:\n#{out}"
      end
    end

    def write_file
      temp_c = Tempfile.create('natalie.c')
      temp_c.write(to_c)
      temp_c.close
      @c_path = temp_c.path
    end

    def to_c
      exprs = @ast.map { |e| compile_expr(e) }.join("\n")
      BOILERPLATE.sub(/__MAIN__/, exprs)
    end

    def compile_expr(expr)
      case expr.first
      when :number
        "NatType *#{next_var_name} = nat_number(#{expr.last});"
      when :string
        "NatType *#{next_var_name} = nat_string(#{expr.last.inspect});"
      else
        raise "unknown AST node: #{expr.inspect}"
      end
    end

    private

    def next_var_name
      @var_num += 1
      "var#{@var_num}"
    end
  end
end
