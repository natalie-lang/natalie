require 'fiddle'
require 'tempfile'

module Natalie
  class Repl
    def go
      to_clean_up = []
      env = nil
      loop do
        break unless (line = get_line)
        ast = Natalie::Parser.new(line).ast
        last_node = ast.pop
        ast << [:send, 'self', 'puts', [[:send, last_node, 'inspect', []]]]
        out = Tempfile.create('natalie.so')
        to_clean_up << out
        Compiler.new(ast).compile(out.path, shared: true)
        lib = Fiddle.dlopen(out.path)
        env ||= Fiddle::Function.new(lib['build_top_env'], [], Fiddle::TYPE_VOIDP).call
        eval_func = Fiddle::Function.new(lib['EVAL'], [Fiddle::TYPE_VOIDP], Fiddle::TYPE_VOIDP)
        eval_func.call(env)
      end
      to_clean_up.each { |f| File.unlink(f.path) }
    end

    private

    def get_line
      print 'nat> '
      line = gets
      if line
        line
      else
        puts
        nil
      end
    end
  end
end
