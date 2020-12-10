require_relative '../spec_helper'

if RUBY_ENGINE == 'natalie'
  def s(*items)
    sexp = Parser::Sexp.new
    items.each do |item|
      sexp << item
    end
    sexp
  end
else
  class Parser
    def self.parse(code)
      node = RubyParser.new.parse(code)
      if node.nil?
        s(:block)
      elsif node.first == :block
        node
      else
        s(:block, node)
      end
    rescue Racc::ParseError, RubyParser::SyntaxError => e
      raise SyntaxError, e.message
    end
  end
end

describe 'Parser' do
  describe '#parse' do
    it 'parses an empty program' do
      Parser.parse('').should == s(:block)
    end

    it 'parses numbers' do
      Parser.parse('1').should == s(:block, s(:lit, 1))
      Parser.parse(' 1234').should == s(:block, s(:lit, 1234))
      Parser.parse('1.5 ').should == s(:block, s(:lit, 1.5))
    end

    it 'parses operator calls' do
      Parser.parse('1 + 3').should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 3)))
      Parser.parse('1+3').should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 3)))
      Parser.parse("1+\n 3").should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 3)))
      Parser.parse('1 - 3').should == s(:block, s(:call, s(:lit, 1), :-, s(:lit, 3)))
      Parser.parse('1 * 3').should == s(:block, s(:call, s(:lit, 1), :*, s(:lit, 3)))
      Parser.parse('1 / 3').should == s(:block, s(:call, s(:lit, 1), :/, s(:lit, 3)))
      Parser.parse('1 * 2 + 3').should == s(:block, s(:call, s(:call, s(:lit, 1), :*, s(:lit, 2)), :+, s(:lit, 3)))
      Parser.parse('1 / 2 - 3').should == s(:block, s(:call, s(:call, s(:lit, 1), :/, s(:lit, 2)), :-, s(:lit, 3)))
      Parser.parse('1 + 2 * 3').should == s(:block, s(:call, s(:lit, 1), :+, s(:call, s(:lit, 2), :*, s(:lit, 3))))
      Parser.parse('1 - 2 / 3').should == s(:block, s(:call, s(:lit, 1), :-, s(:call, s(:lit, 2), :/, s(:lit, 3))))
      Parser.parse('(1 + 2) * 3').should == s(:block, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :*, s(:lit, 3)))
      Parser.parse("(\n1 + 2\n) * 3").should == s(:block, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :*, s(:lit, 3)))
      Parser.parse('(1 - 2) / 3').should == s(:block, s(:call, s(:call, s(:lit, 1), :-, s(:lit, 2)), :/, s(:lit, 3)))
      Parser.parse('(1 + 2) * (3 + 4)').should == s(:block, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :*, s(:call, s(:lit, 3), :+, s(:lit, 4))))
      Parser.parse('"foo" + "bar"').should == s(:block, s(:call, s(:str, "foo"), :+, s(:str, "bar")))
    end

    it 'raises an error if there is a syntax error' do
      # We choose to more closely match what MRI does vs what ruby_parser raises
      if (RUBY_ENGINE == 'natalie')
        -> { Parser.parse("1 + 2\n\n)") }.should raise_error(SyntaxError, "3: syntax error, unexpected ')'")
      else
        -> { Parser.parse("1 + 2\n\n)") }.should raise_error(SyntaxError, "(string):3 :: parse error on value \")\" (tRPAREN)")
      end
    end

    it 'parses strings' do
      Parser.parse('""').should == s(:block, s(:str, ''))
      Parser.parse('"foo"').should == s(:block, s(:str, 'foo'))
      Parser.parse('"this is \"quoted\""').should == s(:block, s(:str, 'this is "quoted"'))
      Parser.parse('"other escaped chars \\\\ \n"').should == s(:block, s(:str, "other escaped chars \\ \n"))
      Parser.parse("''").should == s(:block, s(:str, ''))
      Parser.parse("'foo'").should == s(:block, s(:str, 'foo'))
      Parser.parse("'this is \\'quoted\\''").should == s(:block, s(:str, "this is 'quoted'"))
      Parser.parse("'other escaped chars \\\\ \\n'").should == s(:block, s(:str, "other escaped chars \\ \\n"))
    end

    it 'parses multiple expressions' do
      Parser.parse("1 + 2\n3 + 4").should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:call, s(:lit, 3), :+, s(:lit, 4)))
      Parser.parse("1 + 2;'foo'").should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, "foo"))
    end

    it 'parses assignment' do
      Parser.parse("x = 1").should == s(:block, s(:lasgn, :x, s(:lit, 1)))
      Parser.parse("x = 1 + 2").should == s(:block, s(:lasgn, :x, s(:call, s(:lit, 1), :+, s(:lit, 2))))
      if (RUBY_ENGINE == 'natalie')
        -> { Parser.parse("x =") }.should raise_error(SyntaxError, '1: syntax error, unexpected end-of-input')
      else
        -> { Parser.parse("x =") }.should raise_error(SyntaxError, '(string):1 :: parse error on value false ($end)')
      end
      Parser.parse("@foo = 1").should == s(:block, s(:iasgn, :@foo, s(:lit, 1)))
      Parser.parse("@@abc_123 = 1").should == s(:block, s(:cvdecl, :@@abc_123, s(:lit, 1)))
      Parser.parse("$baz = 1").should == s(:block, s(:gasgn, :$baz, s(:lit, 1)))
      Parser.parse("Constant = 1").should == s(:block, s(:cdecl, :Constant, s(:lit, 1)))
    end

    it 'parses method definition' do
      Parser.parse("def foo\nend").should == s(:block, s(:defn, :foo, s(:args), s(:nil)))
      Parser.parse("def foo;end").should == s(:block, s(:defn, :foo, s(:args), s(:nil)))
      Parser.parse("def foo\n1\nend").should == s(:block, s(:defn, :foo, s(:args), s(:lit, 1)))
      Parser.parse("def foo;1;end").should == s(:block, s(:defn, :foo, s(:args), s(:lit, 1)))
      Parser.parse("def foo;1;2 + 2;'foo';end").should == s(:block, s(:defn, :foo, s(:args), s(:lit, 1), s(:call, s(:lit, 2), :+, s(:lit, 2)), s(:str, 'foo')))
      Parser.parse("def foo x, y\nend").should == s(:block, s(:defn, :foo, s(:args, :x, :y), s(:nil)))
      Parser.parse("def foo x,\ny\nend").should == s(:block, s(:defn, :foo, s(:args, :x, :y), s(:nil)))
      Parser.parse("def foo(x, y)\nend").should == s(:block, s(:defn, :foo, s(:args, :x, :y), s(:nil)))
      Parser.parse("def foo(x, y);end").should == s(:block, s(:defn, :foo, s(:args, :x, :y), s(:nil)))
      Parser.parse("def foo(\nx,\n y\n)\nend").should == s(:block, s(:defn, :foo, s(:args, :x, :y), s(:nil)))
      Parser.parse("def foo(x, y)\n1\n2\nend").should == s(:block, s(:defn, :foo, s(:args, :x, :y), s(:lit, 1), s(:lit, 2)))
    end

    it 'parses method calls vs local variable lookup' do
      Parser.parse("foo").should == s(:block, s(:call, nil, :foo))
      Parser.parse("foo = 1; foo").should == s(:block, s(:lasgn, :foo, s(:lit, 1)), s(:lvar, :foo))
      Parser.parse("foo = 1; def bar; foo; end").should == s(:block, s(:lasgn, :foo, s(:lit, 1)), s(:defn, :bar, s(:args), s(:call, nil, :foo)))
      Parser.parse("@foo = 1; foo").should == s(:block, s(:iasgn, :@foo, s(:lit, 1)), s(:call, nil, :foo))
    end

    xit 'parses method calls with parentheses' do
      Parser.parse("foo()").should == s(:block, s(:call, nil, :foo))
      Parser.parse("foo() + bar()").should == s(:block, s(:call, s(:call, nil, :foo), :+, s(:call, nil, :bar)))
      Parser.parse("foo(1, 'baz')").should == s(:block, s(:call, nil, :foo, s(:lit, 1), s(:str, "baz")))
      Parser.parse("foo(\n1 + 2  ,\n  'baz'  \n )").should == s(:block, s(:call, nil, :foo, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, "baz")))
    end

    xit 'parses method calls without parentheses' do
      Parser.parse("foo").should == s(:block, s(:call, nil, :foo))
      Parser.parse("foo + bar").should == s(:block, s(:call, s(:call, nil, :foo), :+, s(:call, nil, :bar)))
      Parser.parse("foo 1, 'baz'").should == s(:block, s(:call, nil, :foo, s(:lit, 1), s(:str, "baz")))
      Parser.parse("foo 1 + 2  ,\n  'baz'").should == s(:block, s(:call, nil, :foo, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, "baz")))
      Parser.parse("foo 'foo' + 'bar'  ,\n  2").should == s(:block, s(:call, nil, :foo, s(:call, s(:str, "foo"), :+, s(:str, "bar")), s(:lit, 2)))
    end

    xit 'parses class definition' do
      Parser.parse("class Foo\nend").should == s(:block, s(:class, :Foo, nil))
      Parser.parse("class Foo;end").should == s(:block, s(:class, :Foo, nil))
      Parser.parse("class FooBar; 1; 2; end").should == s(:block, s(:class, :FooBar, nil, s(:lit, 1), s(:lit, 2)))
      -> { Parser.parse('class foo;end') }.should raise_error(SyntaxError, 'class/module name must be CONSTANT')
      Parser.parse("class Foo < Bar; 3\n 4\n end").should == s(:block, s(:class, :Foo, s(:const, :Bar), s(:lit, 3), s(:lit, 4)))
    end

    xit 'parses module definition' do
      Parser.parse("module Foo\nend").should == s(:block, s(:module, :Foo))
      Parser.parse("module Foo;end").should == s(:block, s(:module, :Foo))
      Parser.parse("module FooBar; 1; 2; end").should == s(:block, s(:module, :FooBar, s(:lit, 1), s(:lit, 2)))
      -> { Parser.parse('module foo;end') }.should raise_error(SyntaxError, 'class/module name must be CONSTANT')
    end

    xit 'parses an array' do
      Parser.parse("[]").should == s(:block, s(:array))
      Parser.parse("[1]").should == s(:block, s(:array, s(:lit, 1)))
      Parser.parse("['foo']").should == s(:block, s(:array, s(:str, 'foo')))
      Parser.parse("[1, 2, 3]").should == s(:block, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
      Parser.parse("[\n1 , \n2,\n 3]").should == s(:block, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
      -> { Parser.parse('[ , 1]') }.should raise_error(SyntaxError, /\(string\):1 :: parse error on value/)
    end

    xit 'parses a hash' do
      Parser.parse("{}").should == s(:block, s(:hash))
      Parser.parse("{ 1 => 2 }").should == s(:block, s(:hash, s(:lit, 1), s(:lit, 2)))
      Parser.parse("{ foo: 'bar' }").should == s(:block, s(:hash, s(:lit, :foo), s(:str, 'bar')))
      Parser.parse("{ 1 => 2, 'foo' => 'bar' }").should == s(:block, s(:hash, s(:lit, 1), s(:lit, 2), s(:str, 'foo'), s(:str, 'bar')))
      Parser.parse("{\n 1 => \n2,\n 'foo' =>\n'bar'\n}").should == s(:block, s(:hash, s(:lit, 1), s(:lit, 2), s(:str, 'foo'), s(:str, 'bar')))
      Parser.parse("{ foo: 'bar', baz: 'buz' }").should == s(:block, s(:hash, s(:lit, :foo), s(:str, 'bar'), s(:lit, :baz), s(:str, 'buz')))
      -> { Parser.parse('{ , 1 => 2 }') }.should raise_error(SyntaxError, /\(string\):1 :: parse error on value/)
    end

    xit 'parses assignment' do
      Parser.parse("a = 'foo'").should == s(:block, s(:lasgn, :a, s(:str, "foo")))
      Parser.parse("a =\n'foo'").should == s(:block, s(:lasgn, :a, s(:str, "foo")))
      Parser.parse("a = 1").should == s(:block, s(:lasgn, :a, s(:lit, 1)))
    end

    xit 'ignores comments' do
      Parser.parse("# comment").should == s(:block)
      Parser.parse("# comment\n#comment 2").should == s(:block)
      Parser.parse("1 + 1 # comment").should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 1)))
    end
  end
end
