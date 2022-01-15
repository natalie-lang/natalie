# skip-ruby

require 'sexp_processor'

require_relative '../spec_helper'
require_relative '../../lib/natalie/compiler2'

describe 'Natalie::Compiler2' do
  def compile_and_run(path, *args)
    ast = Parser.parse(File.read(path), path)
    compiler = Natalie::Compiler2.new(ast, path)
    compiler.compile
    begin
      out = `#{compiler.out_path} #{args.map(&:inspect).join(' ')}`
      $?.exitstatus.should == 0
    ensure
      File.unlink(compiler.out_path)
    end
    out.strip
  end

  it 'compiles examples/hello.rb' do
    path = File.expand_path('../../examples/hello.rb', __dir__)
    compile_and_run(path).should == 'hello world'
  end

  it 'compiles examples/fib.rb' do
    path = File.expand_path('../../examples/fib.rb', __dir__)
    compile_and_run(path, 6).should == '8'
  end

  it 'executes test/natalie/compiler2/bootstrap_test.rb' do
    path = File.expand_path('compiler2/bootstrap_test.rb', __dir__)
    compile_and_run(path).should == 'all tests successful'
  end
end
