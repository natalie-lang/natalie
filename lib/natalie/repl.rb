require 'fiddle'
require 'tempfile'

begin
  require 'readline'
rescue LoadError
  class Readline
    def self.readline(prompt, _)
      print prompt
      gets
    end
  end
end

module Natalie
  class Repl
    def go
      to_clean_up = []
      env = nil
      vars = {}
      loop do
        break unless (line = get_line)
        ast = Natalie::Parser.new(line, '(repl)').ast
        last_node = ast.pop
        ast << last_node.new(:call, nil, 'puts', s(:call, s(:lasgn, :_, last_node), 'inspect'))
        out = Tempfile.create('natalie.so')
        to_clean_up << out
        compiler = Compiler.new(ast, '(repl)')
        compiler.repl = true
        compiler.vars = vars
        compiler.out_path = out.path
        compiler.compile
        vars = compiler.context[:vars]
        lib = Fiddle.dlopen(out.path)
        env ||= Fiddle::Function.new(lib['build_top_env'], [], Fiddle::TYPE_VOIDP).call
        eval_func = Fiddle::Function.new(lib['EVAL'], [Fiddle::TYPE_VOIDP], Fiddle::TYPE_VOIDP)
        eval_func.call(env)
      end
      to_clean_up.each { |f| File.unlink(f.path) }
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
