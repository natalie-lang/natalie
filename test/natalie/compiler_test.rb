# skip-test

require 'sexp_processor'

require_relative '../spec_helper'
require_relative '../../lib/natalie/compiler'

describe 'Natalie::Compiler' do
  it 'compiles from raw AST to pass1 AST' do
    context = {
      var_prefix: '',
      var_num: 0,
      template: '',
      repl: false,
      vars: {},
      inline_cpp_enabled: false,
      compile_flags: [],
    }

    fib = Parser.parse(File.read(File.expand_path('../../examples/fib.rb', __dir__)), 'examples/fib.rb')

    pass1 = Natalie::Compiler::Pass1.new(context)
    ast = pass1.go(fib)
    actual = ast.inspect.gsub(/\s+/, "\n").strip
    expected = `bin/natalie -d p1 examples/fib.rb`.gsub(/\s+/, "\n").strip
    actual.should == expected

    pass2 = Natalie::Compiler::Pass2.new(context)
    ast = pass2.go(ast)
    actual = ast.inspect.gsub(/\s+/, "\n").strip
    expected = `bin/natalie -d p2 examples/fib.rb`.gsub(/\s+/, "\n").strip
    actual.should == expected

    pass3 = Natalie::Compiler::Pass3.new(context)
    ast = pass3.go(ast)
    actual = ast.inspect.gsub(/\s+/, "\n").strip
    expected = `bin/natalie -d p3 examples/fib.rb`.gsub(/\s+/, "\n").strip
    actual.should == expected

    compiler = Natalie::Compiler.new(fib, 'examples/fib.rb')
    # FIXME: line numbers do not match between our Parser and the ruby_parser gem
    actual = compiler.to_c.gsub(/^.*set_file.*$/, '').strip
    expected = `bin/natalie examples/fib.rb -d`.gsub(/^.*set_file.*$/, '').split('-' * 80).first.strip
    actual.should == expected
  end
end
