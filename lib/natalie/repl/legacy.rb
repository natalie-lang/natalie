require 'fiddle'
require 'fileutils'
require 'tempfile'

if RUBY_ENGINE != 'natalie'
  begin
    require 'readline' if STDOUT.tty?
  rescue LoadError
  end
end

unless defined?(Readline)
  class Readline
    def self.readline(prompt, _)
      print(prompt)
      if !(line = gets)
        exit
      end
      line
    end
  end
end

module Natalie
  class Repl
    def go
      GC.disable
      env = nil
      vars = {}
      repl_num = 0
      multi_line_expr = []
      loop do
        break unless (line = get_line)
        begin
          multi_line_expr << line
          ast = Natalie::Parser.new(multi_line_expr.join("\n"), '(repl)').ast
        rescue Parser::IncompleteExpression
          next
        rescue Racc::ParseError => e
          puts e.message
          multi_line_expr = []
          next
        else
          multi_line_expr = []
        end
        next if ast == s(:block)
        last_node = ast.pop
        ast << last_node.new(:call, nil, 'puts', s(:call, s(:lasgn, :_, last_node), 'inspect'))
        temp = Tempfile.create('natalie.so')
        compiler = Compiler.new(ast, '(repl)')
        compiler.repl = true
        compiler.repl_num = (repl_num += 1)
        compiler.vars = vars
        compiler.out_path = temp.path
        compiler.compile
        vars = compiler.context[:vars]
        lib = Fiddle.dlopen(temp.path)
        Fiddle::Function.new(lib['GC_disable'], [], Fiddle::TYPE_VOIDP).call
        env ||= Fiddle::Function.new(lib['build_top_env'], [], Fiddle::TYPE_VOIDP).call
        eval_func = Fiddle::Function.new(lib['EVAL'], [Fiddle::TYPE_VOIDP], Fiddle::TYPE_VOIDP)
        eval_func.call(env)
        File.unlink(temp.path)
        clang_dir = temp.path + '.dSYM'
        FileUtils.rm_rf(clang_dir) if File.directory?(clang_dir)
      end
    end

    private

    def get_line
      line = Readline.readline('nat> ', true)
      if line
        line
      else
        puts
        nil
      end
    end
  end
end
