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
    def initialize
      @to_clean_up = []
      @var_decls = {}
      # Readline.completion_proc = self.method(:completion)
    end

    def go
      env = nil
      loop do

          break unless (line = get_line)
          out = Tempfile.create('natalie.so')
          do_compile(line, out) 
          lib = Fiddle.dlopen(out.path)
          env ||= Fiddle::Function.new(lib['build_top_env'], [], Fiddle::TYPE_VOIDP).call
          eval_func = Fiddle::Function.new(lib['EVAL'], [Fiddle::TYPE_VOIDP], Fiddle::TYPE_VOIDP)
          eval_func.call(env)
      end
    rescue Interrupt => e
      system('stty', `stty -g`.chomp)
      exit
    ensure
      @to_clean_up.each { |f| File.unlink(f.path) }
    end

    private

    def do_compile(line, outfile)
      ast = Natalie::Parser.new(line, '(repl)').ast
      last_node = ast.pop
      ast << last_node.new(:call, nil, 'puts', s(:call, s(:lasgn, :_, last_node), 'inspect'))
      
      @to_clean_up << outfile
      
      compiler = Compiler.new(ast, '(repl)')
      compiler.repl = true
      compiler.vars = @var_decls
      compiler.out_path = outfile.path
      compiler.compile
      
      @var_decls = compiler.context[:vars]
    end

    def completion(str)
      # TODO
    end

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
