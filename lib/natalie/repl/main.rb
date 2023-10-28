require 'fiddle'
require 'fileutils'
require 'tempfile'
require_relative 'repl'

module Natalie
  class REPL
    def go(options)
      GC.disable
      env = nil
      vars = {}
      repl_num = 0
      driver
        .new(vars)
        .prompt do |cmd|
          begin
            if repl_num.zero? && options[:require].any?
              requires = options[:require].map { |req| "require '#{req}'\n" }
              cmd = requires.join + cmd
            end

            ast = Natalie::Parser.new(cmd, '(repl)').ast
          rescue Parser::IncompleteExpression
            next :continue
          rescue SyntaxError => e
            STDERR.puts e
            next :next
          end

          next :continue if ast.body.empty?
          last_node = ast.body.pop
          ast.body << Prism.call_node(
                        receiver: nil,
                        name: :puts,
                        arguments: [
                          Prism.call_node(receiver: Prism.local_variable_write_node(name: :_, value: last_node), name: :inspect)
                        ]
                      )
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

    private

    def driver
      if $stdin.tty?
        Natalie::GenericRepl
      else
        Natalie::NonTTYRepl
      end
    end

    def s(*items)
      Natalie::Parser::Sexp.new(*items)
    end
  end
end
