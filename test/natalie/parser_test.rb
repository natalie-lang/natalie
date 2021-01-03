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
    def self.parse(code, path = '(string)')
      node = RubyParser.new.parse(code, path)
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

    it 'parses operator expressions' do
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
      Parser.parse('1 == 1').should == s(:block, s(:call, s(:lit, 1), :==, s(:lit, 1)))
      Parser.parse('a != b').should == s(:block, s(:call, s(:call, nil, :a), :!=, s(:call, nil, :b)))
      Parser.parse('Foo === bar').should == s(:block, s(:call, s(:const, :Foo), :===, s(:call, nil, :bar)))
      Parser.parse('1 < 2').should == s(:block, s(:call, s(:lit, 1), :<, s(:lit, 2)))
      Parser.parse('1 <= 2').should == s(:block, s(:call, s(:lit, 1), :<=, s(:lit, 2)))
      Parser.parse('1 > 2').should == s(:block, s(:call, s(:lit, 1), :>, s(:lit, 2)))
      Parser.parse('1 >= 2').should == s(:block, s(:call, s(:lit, 1), :>=, s(:lit, 2)))
      Parser.parse('5-3').should == s(:block, s(:call, s(:lit, 5), :-, s(:lit, 3)))
      Parser.parse('5 -3').should == s(:block, s(:call, s(:lit, 5), :-, s(:lit, 3)))
      Parser.parse('1 +1').should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 1)))
      Parser.parse('(1+2)-3 == 0').should == s(:block, s(:call, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :-, s(:lit, 3)), :==, s(:lit, 0)))
      Parser.parse('2 ** 10').should == s(:block, s(:call, s(:lit, 2), :**, s(:lit, 10)))
      Parser.parse('1 * 2 ** 10 + 3').should == s(:block, s(:call, s(:call, s(:lit, 1), :*, s(:call, s(:lit, 2), :**, s(:lit, 10))), :+, s(:lit, 3)))
      Parser.parse('1 & 2 | 3 ^ 4').should == s(:block, s(:call, s(:call, s(:call, s(:lit, 1), :&, s(:lit, 2)), :|, s(:lit, 3)), :^, s(:lit, 4)))
      Parser.parse('1 && 2 || 3 && 4').should == s(:block, s(:or, s(:and, s(:lit, 1), s(:lit, 2)), s(:and, s(:lit, 3), s(:lit, 4))))
      Parser.parse('1 and 2 or 3 and 4').should == s(:block, s(:and, s(:or, s(:and, s(:lit, 1), s(:lit, 2)), s(:lit, 3)), s(:lit, 4)))
      Parser.parse('10 % 3').should == s(:block, s(:call, s(:lit, 10), :%, s(:lit, 3)))
      Parser.parse('x << 1').should == s(:block, s(:call, s(:call, nil, :x), :<<, s(:lit, 1)))
      Parser.parse('x =~ y').should == s(:block, s(:call, s(:call, nil, :x), :=~, s(:call, nil, :y)))
      Parser.parse('x !~ y').should == s(:block, s(:not, s(:call, s(:call, nil, :x), :=~, s(:call, nil, :y))))
    end

    it 'parses ! and not' do
      Parser.parse('!false').should == s(:block, s(:call, s(:false), :!))
      Parser.parse('not false').should == s(:block, s(:call, s(:false), :!))
      Parser.parse('!foo.bar(baz)').should == s(:block, s(:call, s(:call, s(:call, nil, :foo), :bar, s(:call, nil, :baz)), :!))
    end

    it 'raises an error if there is a syntax error' do
      # We choose to more closely match what MRI does vs what ruby_parser raises
      if (RUBY_ENGINE == 'natalie')
        -> { Parser.parse("1 + 2\n\n)") }.should raise_error(SyntaxError, "(string)#3: syntax error, unexpected ')' (expected: 'end-of-line')")
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
      Parser.parse('"#{:foo} bar #{1 + 1}"').should == s(:block, s(:dstr, "", s(:evstr, s(:lit, :foo)), s(:str, " bar "), s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1)))))
      Parser.parse('y = "#{1 + 1} 2"').should == s(:block, s(:lasgn, :y, s(:dstr, "", s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), s(:str, " 2"))))
      Parser.parse('x.y = "#{1 + 1} 2"').should == s(:block, s(:attrasgn, s(:call, nil, :x), :y=, s(:dstr, "", s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), s(:str, " 2"))))
    end

    it 'parses symbols' do
      Parser.parse(':foo').should == s(:block, s(:lit, :foo))
      Parser.parse(':foo_bar').should == s(:block, s(:lit, :foo_bar))
      Parser.parse(':"foo bar"').should == s(:block, s(:lit, :"foo bar"))
      Parser.parse(':FooBar').should == s(:block, s(:lit, :FooBar))
    end

    it 'parses regular expressions' do
      Parser.parse('/foo/').should == s(:block, s(:lit, /foo/))
    end

    it 'parses multiple expressions' do
      Parser.parse("1 + 2\n3 + 4").should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:call, s(:lit, 3), :+, s(:lit, 4)))
      Parser.parse("1 + 2;'foo'").should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, "foo"))
    end

    it 'parses assignment' do
      Parser.parse("x = 1").should == s(:block, s(:lasgn, :x, s(:lit, 1)))
      Parser.parse("x = 1 + 2").should == s(:block, s(:lasgn, :x, s(:call, s(:lit, 1), :+, s(:lit, 2))))
      if (RUBY_ENGINE == 'natalie')
        -> { Parser.parse("x =") }.should raise_error(SyntaxError, "(string)#1: syntax error, unexpected end-of-input (expected: 'expression')")
        -> { Parser.parse("[1] = 2") }.should raise_error(SyntaxError, "(string)#1: syntax error, unexpected '[' (expected: 'left side of assignment')")
      else
        -> { Parser.parse("x =") }.should raise_error(SyntaxError, '(string):1 :: parse error on value false ($end)')
        -> { Parser.parse("[1] = 2") }.should raise_error(SyntaxError, '(string):1 :: parse error on value "=" (tEQL)')
      end
      Parser.parse("@foo = 1").should == s(:block, s(:iasgn, :@foo, s(:lit, 1)))
      Parser.parse("@@abc_123 = 1").should == s(:block, s(:cvdecl, :@@abc_123, s(:lit, 1)))
      Parser.parse("$baz = 1").should == s(:block, s(:gasgn, :$baz, s(:lit, 1)))
      Parser.parse("Constant = 1").should == s(:block, s(:cdecl, :Constant, s(:lit, 1)))
      Parser.parse("x, y = [1, 2]").should == s(:block, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2)))))
      Parser.parse("(x, y) = [1, 2]").should == s(:block, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2)))))
      Parser.parse("@x, $y, Z = foo").should == s(:block, s(:masgn, s(:array, s(:iasgn, :@x), s(:gasgn, :$y), s(:cdecl, :Z)), s(:to_ary, s(:call, nil, :foo))))
      Parser.parse("(@x, $y, Z) = foo").should == s(:block, s(:masgn, s(:array, s(:iasgn, :@x), s(:gasgn, :$y), s(:cdecl, :Z)), s(:to_ary, s(:call, nil, :foo))))
      Parser.parse("(a, (b, c)) = [1, [2, 3]]").should == s(:block, s(:masgn, s(:array, s(:lasgn, :a), s(:masgn, s(:array, s(:lasgn, :b), s(:lasgn, :c)))), s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3))))))
      Parser.parse("a, (b, c) = [1, [2, 3]]").should == s(:block, s(:masgn, s(:array, s(:lasgn, :a), s(:masgn, s(:array, s(:lasgn, :b), s(:lasgn, :c)))), s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3))))))
    end

    it 'parses attr assignment' do
      Parser.parse("x.y = 1").should == s(:block, s(:attrasgn, s(:call, nil, :x), :y=, s(:lit, 1)))
      Parser.parse("x[y] = 1").should == s(:block, s(:attrasgn, s(:call, nil, :x), :[]=, s(:call, nil, :y), s(:lit, 1)))
    end

    it 'parses [] as an array vs as a method' do
      Parser.parse("foo[1]").should == s(:block, s(:call, s(:call, nil, :foo), :[], s(:lit, 1)))
      Parser.parse("foo [1]").should == s(:block, s(:call, nil, :foo, s(:array, s(:lit, 1))))
      Parser.parse("foo = []; foo[1]").should == s(:block, s(:lasgn, :foo, s(:array)), s(:call, s(:lvar, :foo), :[], s(:lit, 1)))
      Parser.parse("foo = [1]; foo[1]").should == s(:block, s(:lasgn, :foo, s(:array, s(:lit, 1))), s(:call, s(:lvar, :foo), :[], s(:lit, 1)))
      Parser.parse("foo = []; foo [1]").should == s(:block, s(:lasgn, :foo, s(:array)), s(:call, s(:lvar, :foo), :[], s(:lit, 1)))
      Parser.parse("foo = []; foo [1, 2]").should == s(:block, s(:lasgn, :foo, s(:array)), s(:call, s(:lvar, :foo), :[], s(:lit, 1), s(:lit, 2)))
      Parser.parse("foo[a]").should == s(:block, s(:call, s(:call, nil, :foo), :[], s(:call, nil, :a)))
      Parser.parse("foo[]").should == s(:block, s(:call, s(:call, nil, :foo), :[]))
      Parser.parse("foo []").should == s(:block, s(:call, nil, :foo, s(:array)))
      Parser.parse("foo [a, b]").should == s(:block, s(:call, nil, :foo, s(:array, s(:call, nil, :a), s(:call, nil, :b))))
    end

    it 'parses []=' do
      Parser.parse("foo[a, b] = :bar").should == s(:block, s(:attrasgn, s(:call, nil, :foo), :[]=, s(:call, nil, :a), s(:call, nil, :b), s(:lit, :bar)))
      Parser.parse("foo[] = :bar").should == s(:block, s(:attrasgn, s(:call, nil, :foo), :[]=, s(:lit, :bar)))
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
      Parser.parse("def foo n\nn\nend").should == s(:block, s(:defn, :foo, s(:args, :n), s(:lvar, :n)))
      Parser.parse("def foo((a, b), c, (d, e))\nend").should == s(:block, s(:defn, :foo, s(:args, s(:masgn, :a, :b), :c, s(:masgn, :d, :e)), s(:nil)))
    end

    it 'parses method calls vs local variable lookup' do
      Parser.parse("foo").should == s(:block, s(:call, nil, :foo))
      Parser.parse("foo = 1; foo").should == s(:block, s(:lasgn, :foo, s(:lit, 1)), s(:lvar, :foo))
      Parser.parse("foo = 1; def bar; foo; end").should == s(:block, s(:lasgn, :foo, s(:lit, 1)), s(:defn, :bar, s(:args), s(:call, nil, :foo)))
      Parser.parse("@foo = 1; foo").should == s(:block, s(:iasgn, :@foo, s(:lit, 1)), s(:call, nil, :foo))
      Parser.parse("foo, bar = [1, 2]; foo; bar").should == s(:block, s(:masgn, s(:array, s(:lasgn, :foo), s(:lasgn, :bar)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2)))), s(:lvar, :foo), s(:lvar, :bar))
      Parser.parse("(foo, (bar, baz)) = [1, [2, 3]]; foo; bar; baz").should == s(:block, s(:masgn, s(:array, s(:lasgn, :foo), s(:masgn, s(:array, s(:lasgn, :bar), s(:lasgn, :baz)))), s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3))))), s(:lvar, :foo), s(:lvar, :bar), s(:lvar, :baz))
    end

    it 'parses constants' do
      Parser.parse('ARGV').should == s(:block, s(:const, :ARGV))
      Parser.parse('Foo::Bar').should == s(:block, s(:colon2, s(:const, :Foo), :Bar))
      Parser.parse('Foo::Bar::BAZ').should == s(:block, s(:colon2, s(:colon2, s(:const, :Foo), :Bar), :BAZ))
      Parser.parse('x, y = ::Bar').should == s(:block, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:colon3, :Bar))))
      Parser.parse('Foo::bar').should == s(:block, s(:call, s(:const, :Foo), :bar))
      Parser.parse('Foo::bar = 1 + 2').should == s(:block, s(:attrasgn, s(:const, :Foo), :bar=, s(:call, s(:lit, 1), :+, s(:lit, 2))))
      Parser.parse('Foo::bar x, y').should == s(:block, s(:call, s(:const, :Foo), :bar, s(:call, nil, :x), s(:call, nil, :y)))
    end

    it 'parses global variabls' do
      Parser.parse("$foo").should == s(:block, s(:gvar, :$foo))
      Parser.parse("$0").should == s(:block, s(:gvar, :$0))
    end

    it 'parses instance variabls' do
      Parser.parse("@foo").should == s(:block, s(:ivar, :@foo))
    end

    it 'parses class variabls' do
      Parser.parse("@@foo").should == s(:block, s(:cvar, :@@foo))
    end

    it 'parses method calls with parentheses' do
      Parser.parse("foo()").should == s(:block, s(:call, nil, :foo))
      Parser.parse("foo() + bar()").should == s(:block, s(:call, s(:call, nil, :foo), :+, s(:call, nil, :bar)))
      Parser.parse("foo(1, 'baz')").should == s(:block, s(:call, nil, :foo, s(:lit, 1), s(:str, "baz")))
      Parser.parse("foo(a, b)").should == s(:block, s(:call, nil, :foo, s(:call, nil, :a), s(:call, nil, :b)))
      Parser.parse("foo(\n1 + 2  ,\n  'baz'  \n )").should == s(:block, s(:call, nil, :foo, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, "baz")))
      if (RUBY_ENGINE == 'natalie')
        -> { Parser.parse("foo(") }.should raise_error(SyntaxError, "(string)#1: syntax error, unexpected end-of-input (expected: 'expression')")
      else
        -> { Parser.parse("foo(") }.should raise_error(SyntaxError, '(string):1 :: parse error on value false ($end)')
      end
    end

    it 'parses method calls without parentheses' do
      Parser.parse("foo + bar").should == s(:block, s(:call, s(:call, nil, :foo), :+, s(:call, nil, :bar)))
      Parser.parse("foo 1, 'baz'").should == s(:block, s(:call, nil, :foo, s(:lit, 1), s(:str, "baz")))
      Parser.parse("foo 1 + 2").should == s(:block, s(:call, nil, :foo, s(:call, s(:lit, 1), :+, s(:lit, 2))))
      Parser.parse("foo 1 + 2  ,\n  'baz'").should == s(:block, s(:call, nil, :foo, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, "baz")))
      Parser.parse("foo 'foo' + 'bar'  ,\n  2").should == s(:block, s(:call, nil, :foo, s(:call, s(:str, "foo"), :+, s(:str, "bar")), s(:lit, 2)))
      Parser.parse("foo a, b").should == s(:block, s(:call, nil, :foo, s(:call, nil, :a), s(:call, nil, :b)))
    end

    it 'parses method calls with a receiver' do
      Parser.parse("foo.bar").should == s(:block, s(:call, s(:call, nil, :foo), :bar))
      Parser.parse("foo.bar.baz").should == s(:block, s(:call, s(:call, s(:call, nil, :foo), :bar), :baz))
      Parser.parse("foo.bar 1, 2").should == s(:block, s(:call, s(:call, nil, :foo), :bar, s(:lit, 1), s(:lit, 2)))
      Parser.parse("foo.bar(1, 2)").should == s(:block, s(:call, s(:call, nil, :foo), :bar, s(:lit, 1), s(:lit, 2)))
    end

    it 'parses ternary expressions' do
      Parser.parse("1 ? 2 : 3").should == s(:block, s(:if, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
      Parser.parse("foo ?\nbar + baz\n :\n buz / 2").should == s(:block, s(:if, s(:call, nil, :foo), s(:call, s(:call, nil, :bar), :+, s(:call, nil, :baz)), s(:call, s(:call, nil, :buz), :/, s(:lit, 2))))
    end

    it 'parses if/elsif/else' do
      Parser.parse("if true; 1; end").should == s(:block, s(:if, s(:true), s(:lit, 1), nil))
      Parser.parse("if true; 1; 2; end").should == s(:block, s(:if, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), nil))
      Parser.parse("if false; 1; else; 2; end").should == s(:block, s(:if, s(:false), s(:lit, 1), s(:lit, 2)))
      Parser.parse("if false; 1; elsif 1 + 1 == 2; 2; else; 3; end").should == s(:block, s(:if, s(:false), s(:lit, 1), s(:if, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 1)), :==, s(:lit, 2)), s(:lit, 2), s(:lit, 3))))
      Parser.parse("if false; 1; elsif 1 + 1 == 0; 2; 3; elsif false; 4; elsif foo() == 'bar'; 5; 6; else; 7; end").should == s(:block, s(:if, s(:false), s(:lit, 1), s(:if, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 1)), :==, s(:lit, 0)), s(:block, s(:lit, 2), s(:lit, 3)), s(:if, s(:false), s(:lit, 4), s(:if, s(:call, s(:call, nil, :foo), :==, s(:str, 'bar')), s(:block, s(:lit, 5), s(:lit, 6)), s(:lit, 7))))))
    end

    it 'parses unless' do
      Parser.parse("unless false; 1; else; 2; end").should == s(:block, s(:if, s(:false), s(:lit, 2), s(:lit, 1)))
    end

    it 'parses post-conditional if/unless' do
      Parser.parse("true if true").should == s(:block, s(:if, s(:true), s(:true), nil))
      Parser.parse("true unless true").should == s(:block, s(:if, s(:true), nil, s(:true)))
    end

    it 'parses true/false/nil' do
      Parser.parse("true").should == s(:block, s(:true))
      Parser.parse("false").should == s(:block, s(:false))
      Parser.parse("nil").should == s(:block, s(:nil))
    end

    it 'parses examples/fib.rb' do
      fib = File.read(File.expand_path('../../examples/fib.rb', __dir__))
      Parser.parse(fib).should == s(:block,
                                    s(:defn, :fib, s(:args, :n),
                                      s(:if, s(:call, s(:lvar, :n), :==, s(:lit, 0)),
                                        s(:lit, 0),
                                        s(:if, s(:call, s(:lvar, :n), :==, s(:lit, 1)),
                                          s(:lit, 1),
                                          s(:call,
                                            s(:call, nil, :fib,
                                              s(:call, s(:lvar, :n), :-, s(:lit, 1))),
                                            :+,
                                            s(:call, nil, :fib,
                                              s(:call, s(:lvar, :n), :-, s(:lit, 2))))))),
                                    s(:lasgn, :num, s(:call, s(:const, :ARGV), :first)),
                                    s(:call, nil, :puts,
                                      s(:call, nil, :fib,
                                        s(:if, s(:lvar, :num),
                                          s(:call, s(:lvar, :num), :to_i),
                                          s(:lit, 25)))))
    end

    it 'parses class definition' do
      Parser.parse("class Foo\nend").should == s(:block, s(:class, :Foo, nil))
      Parser.parse("class Foo;end").should == s(:block, s(:class, :Foo, nil))
      Parser.parse("class FooBar; 1; 2; end").should == s(:block, s(:class, :FooBar, nil, s(:lit, 1), s(:lit, 2)))
      -> { Parser.parse('class foo;end') }.should raise_error(SyntaxError, 'class/module name must be CONSTANT')
      Parser.parse("class Foo < Bar; 3\n 4\n end").should == s(:block, s(:class, :Foo, s(:const, :Bar), s(:lit, 3), s(:lit, 4)))
      Parser.parse("class Foo < bar; 3\n 4\n end").should == s(:block, s(:class, :Foo, s(:call, nil, :bar), s(:lit, 3), s(:lit, 4)))
    end

    it 'parses module definition' do
      Parser.parse("module Foo\nend").should == s(:block, s(:module, :Foo))
      Parser.parse("module Foo;end").should == s(:block, s(:module, :Foo))
      Parser.parse("module FooBar; 1; 2; end").should == s(:block, s(:module, :FooBar, s(:lit, 1), s(:lit, 2)))
      -> { Parser.parse('module foo;end') }.should raise_error(SyntaxError, 'class/module name must be CONSTANT')
    end

    it 'parses an array' do
      Parser.parse("[]").should == s(:block, s(:array))
      Parser.parse("[1]").should == s(:block, s(:array, s(:lit, 1)))
      Parser.parse("['foo']").should == s(:block, s(:array, s(:str, 'foo')))
      Parser.parse("[1, 2, 3]").should == s(:block, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
      Parser.parse("[x, y, z]").should == s(:block, s(:array, s(:call, nil, :x), s(:call, nil, :y), s(:call, nil, :z)))
      Parser.parse("[\n1 , \n2,\n 3]").should == s(:block, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
      if (RUBY_ENGINE == 'natalie')
        -> { Parser.parse('[ , 1]') }.should raise_error(SyntaxError, "(string)#1: syntax error, unexpected ',' (expected: 'expression')")
      else
        -> { Parser.parse('[ , 1]') }.should raise_error(SyntaxError, '(string):1 :: parse error on value "," (tCOMMA)')
      end
    end

    it 'parses word arrays' do
      Parser.parse('%w[]').should == s(:block, s(:array))
      Parser.parse("%w[  1 2\t  3\n \n4 ]").should == s(:block, s(:array, s(:str, "1"), s(:str, "2"), s(:str, "3"), s(:str, "4")))
      Parser.parse("%W[  1 2\t  3\n \n4 ]").should == s(:block, s(:array, s(:str, "1"), s(:str, "2"), s(:str, "3"), s(:str, "4")))
      Parser.parse("%i[ foo bar ]").should == s(:block, s(:array, s(:lit, :foo), s(:lit, :bar)))
      Parser.parse("%I[ foo bar ]").should == s(:block, s(:array, s(:lit, :foo), s(:lit, :bar)))
    end

    it 'parses a hash' do
      Parser.parse("{}").should == s(:block, s(:hash))
      Parser.parse("{ 1 => 2 }").should == s(:block, s(:hash, s(:lit, 1), s(:lit, 2)))
      Parser.parse("{ foo: 'bar' }").should == s(:block, s(:hash, s(:lit, :foo), s(:str, 'bar')))
      Parser.parse("{ 1 => 2, 'foo' => 'bar' }").should == s(:block, s(:hash, s(:lit, 1), s(:lit, 2), s(:str, 'foo'), s(:str, 'bar')))
      Parser.parse("{\n 1 => \n2,\n 'foo' =>\n'bar'\n}").should == s(:block, s(:hash, s(:lit, 1), s(:lit, 2), s(:str, 'foo'), s(:str, 'bar')))
      Parser.parse("{ foo: 'bar', baz: 'buz' }").should == s(:block, s(:hash, s(:lit, :foo), s(:str, 'bar'), s(:lit, :baz), s(:str, 'buz')))
      Parser.parse("{ a => b, c => d }").should == s(:block, s(:hash, s(:call, nil, :a), s(:call, nil, :b), s(:call, nil, :c), s(:call, nil, :d)))
      if (RUBY_ENGINE == 'natalie')
        -> { Parser.parse('{ , 1 => 2 }') }.should raise_error(SyntaxError, "(string)#1: syntax error, unexpected ',' (expected: 'expression')")
      else
        -> { Parser.parse('{ , 1 => 2 }') }.should raise_error(SyntaxError, '(string):1 :: parse error on value "," (tCOMMA)')
      end
    end

    it 'ignores comments' do
      Parser.parse("# comment").should == s(:block)
      Parser.parse("# comment\n#comment 2").should == s(:block)
      Parser.parse("1 + 1 # comment").should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 1)))
    end

    it 'parses a range' do
      Parser.parse('1..10').should == s(:block, s(:lit, 1..10))
      Parser.parse("1..'a'").should == s(:block, s(:dot2, s(:lit, 1), s(:str, 'a')))
      Parser.parse("'a'..1").should == s(:block, s(:dot2, s(:str, 'a'), s(:lit, 1)))
      Parser.parse('1...10').should == s(:block, s(:lit, 1...10))
      Parser.parse('1.1..2.2').should == s(:block, s(:dot2, s(:lit, 1.1), s(:lit, 2.2)))
      Parser.parse('1.1...2.2').should == s(:block, s(:dot3, s(:lit, 1.1), s(:lit, 2.2)))
      Parser.parse("'a'..'z'").should == s(:block, s(:dot2, s(:str, "a"), s(:str, "z")))
      Parser.parse("'a'...'z'").should == s(:block, s(:dot3, s(:str, "a"), s(:str, "z")))
      Parser.parse("('a')..('z')").should == s(:block, s(:dot2, s(:str, "a"), s(:str, "z")))
      Parser.parse("('a')...('z')").should == s(:block, s(:dot3, s(:str, "a"), s(:str, "z")))
      Parser.parse("foo..bar").should == s(:block, s(:dot2, s(:call, nil, :foo), s(:call, nil, :bar)))
      Parser.parse("foo...bar").should == s(:block, s(:dot3, s(:call, nil, :foo), s(:call, nil, :bar)))
      Parser.parse("foo = 1..10").should == s(:block, s(:lasgn, :foo, s(:lit, 1..10)))
    end

    it 'parses return' do
      Parser.parse('return').should == s(:block, s(:return))
      Parser.parse('return foo').should == s(:block, s(:return, s(:call, nil, :foo)))
      Parser.parse('return foo if true').should == s(:block, s(:if, s(:true), s(:return, s(:call, nil, :foo)), nil))
    end

    it 'parses block' do
      Parser.parse("foo do\nend").should == s(:block, s(:iter, s(:call, nil, :foo), 0))
      Parser.parse("foo do\n1\n2\nend").should == s(:block, s(:iter, s(:call, nil, :foo), 0, s(:block, s(:lit, 1), s(:lit, 2))))
      Parser.parse("foo do |x, y|\nx\ny\nend").should == s(:block, s(:iter, s(:call, nil, :foo), s(:args, :x, :y), s(:block, s(:lvar, :x), s(:lvar, :y))))
      Parser.parse("foo(a, b) do |x, y|\nx\ny\nend").should == s(:block, s(:iter, s(:call, nil, :foo, s(:call, nil, :a), s(:call, nil, :b)), s(:args, :x, :y), s(:block, s(:lvar, :x), s(:lvar, :y))))
      Parser.parse("foo { }").should == s(:block, s(:iter, s(:call, nil, :foo), 0))
      Parser.parse("foo { 1; 2 }").should == s(:block, s(:iter, s(:call, nil, :foo), 0, s(:block, s(:lit, 1), s(:lit, 2))))
      Parser.parse("foo { |x, y| x; y }").should == s(:block, s(:iter, s(:call, nil, :foo), s(:args, :x, :y), s(:block, s(:lvar, :x), s(:lvar, :y))))
      Parser.parse("foo { |x| x }; x").should == s(:block, s(:iter, s(:call, nil, :foo), s(:args, :x), s(:lvar, :x)), s(:call, nil, :x))
      Parser.parse("x = 1; foo { x }; x").should == s(:block, s(:lasgn, :x, s(:lit, 1)), s(:iter, s(:call, nil, :foo), 0, s(:lvar, :x)), s(:lvar, :x))
      Parser.parse("foo do\nbar do\nend\nend").should == s(:block, s(:iter, s(:call, nil, :foo), 0, s(:iter, s(:call, nil, :bar), 0)))
      Parser.parse("foo do |(x, y), z|\nend").should == s(:block, s(:iter, s(:call, nil, :foo), s(:args, s(:masgn, :x, :y), :z)))
    end

    it 'parses block pass' do
      Parser.parse('map(&:foo)').should == s(:block, s(:call, nil, :map, s(:block_pass, s(:lit, :foo))))
      Parser.parse('map(&block)').should == s(:block, s(:call, nil, :map, s(:block_pass, s(:call, nil, :block))))
    end

    it 'parses next, break, and yield' do
      Parser.parse('next').should == s(:block, s(:next))
      Parser.parse('next 1, 2').should == s(:block, s(:next, s(:array, s(:lit, 1), s(:lit, 2))))
      Parser.parse('next([1, 2])').should == s(:block, s(:next, s(:array, s(:lit, 1), s(:lit, 2))))
      Parser.parse("next if true").should == s(:block, s(:if, s(:true), s(:next), nil))
      Parser.parse('break').should == s(:block, s(:break))
      Parser.parse('break 1, 2').should == s(:block, s(:break, s(:array, s(:lit, 1), s(:lit, 2))))
      Parser.parse('break([1, 2])').should == s(:block, s(:break, s(:array, s(:lit, 1), s(:lit, 2))))
      Parser.parse("break if true").should == s(:block, s(:if, s(:true), s(:break), nil))
      Parser.parse('yield').should == s(:block, s(:yield))
      Parser.parse('yield 1, 2').should == s(:block, s(:yield, s(:lit, 1), s(:lit, 2)))
      Parser.parse('yield(1, 2)').should == s(:block, s(:yield, s(:lit, 1), s(:lit, 2)))
      Parser.parse("yield if true").should == s(:block, s(:if, s(:true), s(:yield), nil))
    end

    it 'parses __FILE__ and __dir__' do
      Parser.parse('__FILE__', 'foo/bar.rb').should == s(:block, s(:str, 'foo/bar.rb'))
      Parser.parse('__dir__').should == s(:block, s(:call, nil, :__dir__))
    end

    it 'parses splat *' do
      Parser.parse('foo(*args)').should == s(:block, s(:call, nil, :foo, s(:splat, s(:call, nil, :args))))
    end

    it 'parses stabby proc' do
      Parser.parse('-> { puts 1 }').should == s(:block, s(:iter, s(:lambda), 0, s(:call, nil, :puts, s(:lit, 1))))
      Parser.parse('-> x { puts x }').should == s(:block, s(:iter, s(:lambda), s(:args, :x), s(:call, nil, :puts, s(:lvar, :x))))
      Parser.parse('-> x, y { puts x, y }').should == s(:block, s(:iter, s(:lambda), s(:args, :x, :y), s(:call, nil, :puts, s(:lvar, :x), s(:lvar, :y))))
      Parser.parse('-> (x, y) { x; y }').should == s(:block, s(:iter, s(:lambda), s(:args, :x, :y), s(:block, s(:lvar, :x), s(:lvar, :y))))
      -> { Parser.parse('->') }.should raise_error(SyntaxError)
    end

    it 'parses case/when/else' do
      Parser.parse("case 1\nwhen 1\n:a\nend").should == s(:block, s(:case, s(:lit, 1), s(:when, s(:array, s(:lit, 1)), s(:lit, :a)), nil))
      Parser.parse("case 1\nwhen 1\n:a\n:b\nwhen 2, 3\n:c\nelse\n:d\nend").should == s(:block, s(:case, s(:lit, 1), s(:when, s(:array, s(:lit, 1)), s(:lit, :a), s(:lit, :b)), s(:when, s(:array, s(:lit, 2), s(:lit, 3)), s(:lit, :c)), s(:lit, :d)))
    end

    it 'parses begin/rescue/else/ensure' do
      Parser.parse("begin;1;2;rescue;3;4;end").should == s(:block, s(:rescue, s(:block, s(:lit, 1), s(:lit, 2)), s(:resbody, s(:array), s(:lit, 3), s(:lit, 4))))
      Parser.parse("begin;1;rescue => e;3;end").should == s(:block, s(:rescue, s(:lit, 1), s(:resbody, s(:array, s(:lasgn, :e, s(:gvar, :$!))), s(:lit, 3))))
      Parser.parse("begin;rescue SyntaxError, NoMethodError => e;3;end").should == s(:block, s(:rescue, s(:resbody, s(:array, s(:const, :SyntaxError), s(:const, :NoMethodError), s(:lasgn, :e, s(:gvar, :$!))), s(:lit, 3))))
      Parser.parse("begin;rescue SyntaxError;3;else;4;5;end").should == s(:block, s(:rescue, s(:resbody, s(:array, s(:const, :SyntaxError)), s(:lit, 3)), s(:block, s(:lit, 4), s(:lit, 5))))
      Parser.parse("begin\n0\nensure\n:a\n:b\nend").should == s(:block, s(:ensure, s(:lit, 0), s(:block, s(:lit, :a), s(:lit, :b))))
      Parser.parse("begin;0;rescue;:a;else;:c;ensure;:d;:e;end").should == s(:block, s(:ensure, s(:rescue, s(:lit, 0), s(:resbody, s(:array), s(:lit, :a)), s(:lit, :c)), s(:block, s(:lit, :d), s(:lit, :e))))
      Parser.parse("begin;0;rescue foo(1), bar(2);1;end").should == s(:block, s(:rescue, s(:lit, 0), s(:resbody, s(:array, s(:call, nil, :foo, s(:lit, 1)), s(:call, nil, :bar, s(:lit, 2))), s(:lit, 1))))
    end

    it 'parses backticks and %x()' do
      Parser.parse("`ls`").should == s(:block, s(:xstr, "ls"))
      Parser.parse("%x(ls)").should == s(:block, s(:xstr, "ls"))
      Parser.parse("%x(ls \#{path})").should == s(:block, s(:dxstr, "ls ", s(:evstr, s(:call, nil, :path))))
    end
  end
end
