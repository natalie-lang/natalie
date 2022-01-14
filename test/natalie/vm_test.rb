# skip-ruby

require 'sexp_processor'

require_relative '../spec_helper'
require_relative '../../lib/natalie/compiler2'
require_relative '../../lib/natalie/vm'

describe 'Natalie::VM' do
  def compile_and_run(path, *args)
    ast = Parser.parse(File.read(path), path)
    compiler = Natalie::Compiler2.new(ast, path, interpret: true)
    vm = Natalie::VM.new(compiler.instructions)
    capture do
      vm.run
    end
  end

  def capture
    stub = IOStub.new
    begin
      old_stdout = $stdout
      $stdout = stub
      yield
    ensure
      $stdout = old_stdout
    end
    stub.to_s.strip
  end

  it 'executes examples/hello.rb' do
    path = File.expand_path('../../examples/hello.rb', __dir__)
    compile_and_run(path).should == 'hello world'
  end

  it 'executes examples/fib.rb' do
    path = File.expand_path('../../examples/fib.rb', __dir__)
    # TODO: maybe a way to pass args to the VM without messing with the host process' ARGV?
    ARGV.clear
    ARGV << '6'
    compile_and_run(path, 6).should == '8'
  end
end
