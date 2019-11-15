require 'minitest/spec'
require 'minitest/autorun'
require_relative '../../lib/natalie'

describe Natalie::Compiler do
  describe '#compile_expr' do
    before do
      @compiler = Natalie::Compiler.new
    end

    it 'compiles a number AST expression to C code' do
      @compiler.compile_expr([:number, '1']).must_equal 'NatType *var1 = nat_number(1);'
    end

    it 'compiles a string AST expression to C code' do
      @compiler.compile_expr([:string, 'a']).must_equal 'NatType *var1 = nat_string("a");'
    end

    #it 'compiles an assignment AST expression to C code' do
    #  @compiler.compile_expr([:assign, 'num', [:number, '1']]).must_equal(
    #    "Number *var1 = { .value=1 };\n" \
    #    "env_set(env, \"num\", var1);"
    #  )
    #end
  end

  describe '#compile' do
    before do
      @compiler = Natalie::Compiler.new([[:number, '1']])
      temp = Tempfile.create('test_natalie')
      temp.close
      @out = temp.path
      @compiler.compile(@out)
    end

    it 'writes a C file for compilation' do
      result = `#{@out}`
      result.must_match('hello')
    end
  end
end
