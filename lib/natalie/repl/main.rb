require 'fiddle'
require 'fileutils'
require 'tempfile'
require_relative 'repl'

module Natalie
  class ExperimentalReplV2
    def go
      GC.disable
      env = nil
      vars = {}
      repl_num = 0
      multi_line_expr = []
      Natalie::GenericRepl
        .new(vars)
        .get_command do |cmd|
          begin
            ast = Natalie::Parser.new(cmd, '(repl)').ast
          rescue Parser::IncompleteExpression
            next :continue
          end

          next :continue if ast == s(:block)
          puts "\n\n"
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
          next :next
        end
    end
  end
end
