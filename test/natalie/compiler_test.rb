# skip-ruby

require 'sexp_processor'

require_relative '../spec_helper'
require_relative '../../lib/natalie/compiler'

describe 'Natalie::Compiler' do
  it 'compiles the AST in passes' do
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
    ast = NatalieParser.parse(File.read(path), path)

    pass1 = Natalie::Compiler::Pass1.new(context)
    ast1 = pass1.go(ast)
    actual = ast1.inspect.gsub(/\s+/, "\n").strip
    expected = `#{NAT_BINARY} -d p1 #{path}`.gsub(/\s+/, "\n").strip
    actual.should == expected

    pass1b = Natalie::Compiler::Pass1b.new(context)
    ast1b = pass1b.go(ast1)
    actual = ast1b.inspect.gsub(/\s+/, "\n").strip
    expected = `#{NAT_BINARY} -d p1b #{path}`.gsub(/\s+/, "\n").strip
    actual.should == expected

    pass1r = Natalie::Compiler::Pass1r.new(context)
    ast1r = pass1r.go(ast1b)
    actual = ast1r.inspect.gsub(/\s+/, "\n").strip
    expected = `#{NAT_BINARY} -d p1r #{path}`.gsub(/\s+/, "\n").strip
    actual.should == expected

    pass2 = Natalie::Compiler::Pass2.new(context)
    ast2 = pass2.go(ast1r)
    actual = ast2.inspect.gsub(/\s+/, "\n").strip
    expected = `#{NAT_BINARY} -d p2 #{path}`.gsub(/\s+/, "\n").strip
    actual.should == expected

    pass3 = Natalie::Compiler::Pass3.new(context)
    ast3 = pass3.go(ast2)
    actual = ast3.inspect.gsub(/\s+/, "\n").strip
    expected = `#{NAT_BINARY} -d p3 #{path}`.gsub(/\s+/, "\n").strip
    actual.should == expected

    compiler = Natalie::Compiler.new(ast, path)

    actual = compiler.instructions.strip
    expected = `#{NAT_BINARY} #{path} -d`.split('-' * 80).first.strip
    actual.should == expected
  end
end
