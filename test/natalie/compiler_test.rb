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

    path = File.expand_path('../../examples/boardslam.rb', __dir__)
    ast = Parser.parse(File.read(path), path)

    pass1 = Natalie::Compiler::Pass1.new(context)
    ast1 = pass1.go(ast)
    actual = ast1.inspect.gsub(/\s+/, "\n").strip
    expected = `bin/natalie -d p1 #{path}`.gsub(/\s+/, "\n").strip
    actual.should == expected

    pass2 = Natalie::Compiler::Pass2.new(context)
    ast2 = pass2.go(ast1)
    actual = ast2.inspect.gsub(/\s+/, "\n").strip
    expected = `bin/natalie -d p2 #{path}`.gsub(/\s+/, "\n").strip
    actual.should == expected

    pass3 = Natalie::Compiler::Pass3.new(context)
    ast3 = pass3.go(ast2)
    actual = ast3.inspect.gsub(/\s+/, "\n").strip
    expected = `bin/natalie -d p3 #{path}`.gsub(/\s+/, "\n").strip
    actual.should == expected

    compiler = Natalie::Compiler.new(ast, path)
    # FIXME: line numbers do not match between our Parser and the ruby_parser gem
    actual = compiler.to_c.gsub(/^.*set_line.*\n/, '').strip
    expected = `bin/natalie #{path} -d`.gsub(/^.*set_line.*\n/, '').split('-' * 80).first.strip
    actual.should == expected
  end
end
