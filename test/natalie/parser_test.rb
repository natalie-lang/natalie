require_relative '../spec_helper'
require 'sexp_processor'

if RUBY_ENGINE != 'natalie'
  require 'ruby_parser'
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
      Parser.parse("(\n1 + 2\n) * 3").should ==
        s(:block, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :*, s(:lit, 3)))
      Parser.parse('(1 - 2) / 3').should == s(:block, s(:call, s(:call, s(:lit, 1), :-, s(:lit, 2)), :/, s(:lit, 3)))
      Parser.parse('(1 + 2) * (3 + 4)').should ==
        s(:block, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :*, s(:call, s(:lit, 3), :+, s(:lit, 4))))
      Parser.parse('"foo" + "bar"').should == s(:block, s(:call, s(:str, 'foo'), :+, s(:str, 'bar')))
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
      Parser.parse('(1+2)-3 == 0').should ==
        s(:block, s(:call, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :-, s(:lit, 3)), :==, s(:lit, 0)))
      Parser.parse('2 ** 10').should == s(:block, s(:call, s(:lit, 2), :**, s(:lit, 10)))
      Parser.parse('1 * 2 ** 10 + 3').should ==
        s(:block, s(:call, s(:call, s(:lit, 1), :*, s(:call, s(:lit, 2), :**, s(:lit, 10))), :+, s(:lit, 3)))
      Parser.parse('1 & 2 | 3 ^ 4').should ==
        s(:block, s(:call, s(:call, s(:call, s(:lit, 1), :&, s(:lit, 2)), :|, s(:lit, 3)), :^, s(:lit, 4)))
      Parser.parse('10 % 3').should == s(:block, s(:call, s(:lit, 10), :%, s(:lit, 3)))
      Parser.parse('x << 1').should == s(:block, s(:call, s(:call, nil, :x), :<<, s(:lit, 1)))
      Parser.parse('x =~ y').should == s(:block, s(:call, s(:call, nil, :x), :=~, s(:call, nil, :y)))
      Parser.parse('x =~ /foo/').should == s(:block, s(:match3, s(:lit, /foo/), s(:call, nil, :x)))
      Parser.parse('/foo/ =~ x').should == s(:block, s(:match2, s(:lit, /foo/), s(:call, nil, :x)))
      Parser.parse('x !~ y').should == s(:block, s(:not, s(:call, s(:call, nil, :x), :=~, s(:call, nil, :y))))
      Parser.parse('x !~ /foo/').should == s(:block, s(:not, s(:match3, s(:lit, /foo/), s(:call, nil, :x))))
      Parser.parse('/foo/ !~ x').should == s(:block, s(:not, s(:match2, s(:lit, /foo/), s(:call, nil, :x))))
      Parser.parse('x &&= 1').should == s(:block, s(:op_asgn_and, s(:lvar, :x), s(:lasgn, :x, s(:lit, 1))))
      Parser.parse('x ||= 1').should == s(:block, s(:op_asgn_or, s(:lvar, :x), s(:lasgn, :x, s(:lit, 1))))
      Parser.parse('x += 1').should == s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :+, s(:lit, 1))))
      Parser.parse('x -= 1').should == s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :-, s(:lit, 1))))
      Parser.parse('x *= 1').should == s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :*, s(:lit, 1))))
      Parser.parse('x **= 1').should == s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :**, s(:lit, 1))))
      Parser.parse('x /= 1').should == s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :/, s(:lit, 1))))
      Parser.parse('x %= 1').should == s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :%, s(:lit, 1))))
      Parser.parse('x |= 1').should == s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :|, s(:lit, 1))))
      Parser.parse('x &= 1').should == s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :&, s(:lit, 1))))
      Parser.parse('x >>= 1').should == s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :>>, s(:lit, 1))))
      Parser.parse('x <<= 1').should == s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :<<, s(:lit, 1))))
      Parser.parse('n = x += 1').should ==
        s(:block, s(:lasgn, :n, s(:lasgn, :x, s(:call, s(:lvar, :x), :+, s(:lit, 1)))))
      Parser.parse('x.y += 1').should == s(:block, s(:op_asgn2, s(:call, nil, :x), :y=, :+, s(:lit, 1)))
      Parser.parse('x[:y] += 1').should ==
        s(:block, s(:op_asgn1, s(:call, nil, :x), s(:arglist, s(:lit, :y)), :+, s(:lit, 1)))
      Parser.parse('foo&.bar').should == s(:block, s(:safe_call, s(:call, nil, :foo), :bar))
      Parser.parse('foo&.bar 1').should == s(:block, s(:safe_call, s(:call, nil, :foo), :bar, s(:lit, 1)))
      Parser.parse('foo&.bar x').should == s(:block, s(:safe_call, s(:call, nil, :foo), :bar, s(:call, nil, :x)))
      Parser.parse('-(2*8)').should == s(:block, s(:call, s(:call, s(:lit, 2), :*, s(:lit, 8)), :-@))
      Parser.parse('+(2*8)').should == s(:block, s(:call, s(:call, s(:lit, 2), :*, s(:lit, 8)), :+@))
      Parser.parse('foo <=> bar').should == s(:block, s(:call, s(:call, nil, :foo), :<=>, s(:call, nil, :bar)))
    end

    it 'parses and/or' do
      Parser.parse('1 && 2 || 3 && 4').should ==
        s(:block, s(:or, s(:and, s(:lit, 1), s(:lit, 2)), s(:and, s(:lit, 3), s(:lit, 4))))
      Parser.parse('false and true and false').should == s(:block, s(:and, s(:false), s(:and, s(:true), s(:false))))
      Parser.parse('false or true or false').should == s(:block, s(:or, s(:false), s(:or, s(:true), s(:false))))
      Parser.parse('false && true && false').should == s(:block, s(:and, s(:false), s(:and, s(:true), s(:false))))
      Parser.parse('false || true || false').should == s(:block, s(:or, s(:false), s(:or, s(:true), s(:false))))
      Parser.parse('1 and 2 or 3 and 4').should ==
        s(:block, s(:and, s(:or, s(:and, s(:lit, 1), s(:lit, 2)), s(:lit, 3)), s(:lit, 4)))
      Parser.parse('1 or 2 and 3 or 4').should ==
        s(:block, s(:or, s(:and, s(:or, s(:lit, 1), s(:lit, 2)), s(:lit, 3)), s(:lit, 4)))
    end

    it 'parses ! and not' do
      Parser.parse('!false').should == s(:block, s(:call, s(:false), :!))
      Parser.parse('not false').should == s(:block, s(:call, s(:false), :!))
      Parser.parse('!foo.bar(baz)').should ==
        s(:block, s(:call, s(:call, s(:call, nil, :foo), :bar, s(:call, nil, :baz)), :!))
    end

    it 'raises an error if there is a syntax error' do
      # We choose to more closely match what MRI does vs what ruby_parser raises
      if (RUBY_ENGINE == 'natalie')
        -> { Parser.parse("1 + 2\n\n)") }.should raise_error(
                                                   SyntaxError,
                                                   "(string)#3: syntax error, unexpected ')' (expected: 'end-of-line')",
                                                 )
      else
        -> { Parser.parse("1 + 2\n\n)") }.should raise_error(
                                                   SyntaxError,
                                                   "(string):3 :: parse error on value \")\" (tRPAREN)",
                                                 )
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
      Parser.parse('"#{:foo} bar #{1 + 1}"').should ==
        s(
          :block,
          s(:dstr, '', s(:evstr, s(:lit, :foo)), s(:str, ' bar '), s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1)))),
        )
      Parser.parse('y = "#{1 + 1} 2"').should ==
        s(:block, s(:lasgn, :y, s(:dstr, '', s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), s(:str, ' 2'))))
      Parser.parse('x.y = "#{1 + 1} 2"').should ==
        s(
          :block,
          s(
            :attrasgn,
            s(:call, nil, :x),
            :y=,
            s(:dstr, '', s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), s(:str, ' 2')),
          ),
        )
      Parser.parse(%("\#{1}foo\#{''}bar")).should ==
        s(:block, s(:dstr, '', s(:evstr, s(:lit, 1)), s(:str, 'foo'), s(:str, ''), s(:str, 'bar')))
    end

    it 'parses symbols' do
      Parser.parse(':foo').should == s(:block, s(:lit, :foo))
      Parser.parse(':foo_bar').should == s(:block, s(:lit, :foo_bar))
      Parser.parse(':"foo bar"').should == s(:block, s(:lit, :'foo bar'))
      Parser.parse(':FooBar').should == s(:block, s(:lit, :FooBar))
    end

    it 'parses regexps' do
      Parser.parse('/foo/').should == s(:block, s(:lit, /foo/))
      Parser.parse('/foo/i').should == s(:block, s(:lit, /foo/i))
      Parser.parse('//mix').should == s(:block, s(:lit, //mix))
      Parser.parse('/#{1+1}/mix').should == s(:block, s(:dregx, '', s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), 7))
      Parser.parse('/foo #{1+1}/').should ==
        s(:block, s(:dregx, 'foo ', s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1)))))
      Parser.parse('/^$(.)[.]{1}.*.+.?\^\$\.\(\)\[\]\{\}\w\W\d\D\h\H\s\S\R\*\+\?/').should ==
        s(:block, s(:lit, /^$(.)[.]{1}.*.+.?\^\$\.\(\)\[\]\{\}\w\W\d\D\h\H\s\S\R\*\+\?/))
      Parser.parse("/\\n\\\\n/").should == s(:block, s(:lit, /\n\\n/))
      Parser.parse("/\\/\\* foo \\*\\//").should == s(:block, s(:lit, Regexp.new("\\/\\* foo \\*\\/")))
      Parser.parse("/\\&\\a\\b/").should == s(:block, s(:lit, /\&\a\b/))
    end

    it 'parses multiple expressions' do
      Parser.parse("1 + 2\n3 + 4").should ==
        s(:block, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:call, s(:lit, 3), :+, s(:lit, 4)))
      Parser.parse("1 + 2;'foo'").should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, 'foo'))
    end

    it 'parses assignment' do
      Parser.parse('x = 1').should == s(:block, s(:lasgn, :x, s(:lit, 1)))
      Parser.parse('x = 1 + 2').should == s(:block, s(:lasgn, :x, s(:call, s(:lit, 1), :+, s(:lit, 2))))
      if (RUBY_ENGINE == 'natalie')
        -> { Parser.parse('x =') }.should raise_error(
                                            SyntaxError,
                                            "(string)#1: syntax error, unexpected end-of-input (expected: 'expression')",
                                          )
        -> { Parser.parse('[1] = 2') }.should raise_error(
                                                SyntaxError,
                                                "(string)#1: syntax error, unexpected '[' (expected: 'left side of assignment')",
                                              )
      else
        -> { Parser.parse('x =') }.should raise_error(SyntaxError, '(string):1 :: parse error on value false ($end)')
        -> { Parser.parse('[1] = 2') }.should raise_error(SyntaxError, '(string):1 :: parse error on value "=" (tEQL)')
      end
      Parser.parse('@foo = 1').should == s(:block, s(:iasgn, :@foo, s(:lit, 1)))
      Parser.parse('@@abc_123 = 1').should == s(:block, s(:cvdecl, :@@abc_123, s(:lit, 1)))
      Parser.parse('$baz = 1').should == s(:block, s(:gasgn, :$baz, s(:lit, 1)))
      Parser.parse('Constant = 1').should == s(:block, s(:cdecl, :Constant, s(:lit, 1)))
      Parser.parse('x, y = [1, 2]').should ==
        s(:block, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2)))))
      Parser.parse('(x, y) = [1, 2]').should ==
        s(:block, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2)))))
      Parser.parse('@x, $y, Z = foo').should ==
        s(:block, s(:masgn, s(:array, s(:iasgn, :@x), s(:gasgn, :$y), s(:cdecl, :Z)), s(:to_ary, s(:call, nil, :foo))))
      Parser.parse('(@x, $y, Z) = foo').should ==
        s(:block, s(:masgn, s(:array, s(:iasgn, :@x), s(:gasgn, :$y), s(:cdecl, :Z)), s(:to_ary, s(:call, nil, :foo))))
      Parser.parse('(a, (b, c)) = [1, [2, 3]]').should ==
        s(
          :block,
          s(
            :masgn,
            s(:array, s(:lasgn, :a), s(:masgn, s(:array, s(:lasgn, :b), s(:lasgn, :c)))),
            s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3)))),
          ),
        )
      Parser.parse('a, (b, c) = [1, [2, 3]]').should ==
        s(
          :block,
          s(
            :masgn,
            s(:array, s(:lasgn, :a), s(:masgn, s(:array, s(:lasgn, :b), s(:lasgn, :c)))),
            s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3)))),
          ),
        )
      Parser.parse('a, (b, *c) = [1, [2, 3, 4]]').should ==
        s(
          :block,
          s(
            :masgn,
            s(:array, s(:lasgn, :a), s(:masgn, s(:array, s(:lasgn, :b), s(:splat, s(:lasgn, :c))))),
            s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3), s(:lit, 4)))),
          ),
        )
      Parser.parse('a, (b, *@c) = [1, [2, 3, 4]]').should ==
        s(
          :block,
          s(
            :masgn,
            s(:array, s(:lasgn, :a), s(:masgn, s(:array, s(:lasgn, :b), s(:splat, s(:iasgn, :@c))))),
            s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3), s(:lit, 4)))),
          ),
        )
      Parser.parse('_, a, *b = [1, 2, 3, 4]').should ==
        s(
          :block,
          s(
            :masgn,
            s(:array, s(:lasgn, :_), s(:lasgn, :a), s(:splat, s(:lasgn, :b))),
            s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3), s(:lit, 4))),
          ),
        )
      Parser.parse('(_, a, *b) = [1, 2, 3, 4]').should ==
        s(
          :block,
          s(
            :masgn,
            s(:array, s(:lasgn, :_), s(:lasgn, :a), s(:splat, s(:lasgn, :b))),
            s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3), s(:lit, 4))),
          ),
        )
      Parser.parse('(a, *) = [1, 2, 3, 4]').should ==
        s(
          :block,
          s(
            :masgn,
            s(:array, s(:lasgn, :a), s(:splat)),
            s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3), s(:lit, 4))),
          ),
        )
      Parser.parse('(a, *, c) = [1, 2, 3, 4]').should ==
        s(
          :block,
          s(
            :masgn,
            s(:array, s(:lasgn, :a), s(:splat), s(:lasgn, :c)),
            s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3), s(:lit, 4))),
          ),
        )
      Parser.parse('x = foo.bar').should == s(:block, s(:lasgn, :x, s(:call, s(:call, nil, :foo), :bar)))
      Parser.parse('x = y = 2').should == s(:block, s(:lasgn, :x, s(:lasgn, :y, s(:lit, 2))))
      Parser.parse('x = y = z = 2').should == s(:block, s(:lasgn, :x, s(:lasgn, :y, s(:lasgn, :z, s(:lit, 2)))))
    end

    it 'parses attr assignment' do
      Parser.parse('x.y = 1').should == s(:block, s(:attrasgn, s(:call, nil, :x), :y=, s(:lit, 1)))
      Parser.parse('x[y] = 1').should == s(:block, s(:attrasgn, s(:call, nil, :x), :[]=, s(:call, nil, :y), s(:lit, 1)))
      Parser.parse('foo[a, b] = :bar').should ==
        s(:block, s(:attrasgn, s(:call, nil, :foo), :[]=, s(:call, nil, :a), s(:call, nil, :b), s(:lit, :bar)))
      Parser.parse('foo[] = :bar').should == s(:block, s(:attrasgn, s(:call, nil, :foo), :[]=, s(:lit, :bar)))
      Parser.parse('foo.bar=x').should == s(:block, s(:attrasgn, s(:call, nil, :foo), :bar=, s(:call, nil, :x)))
    end

    it 'parses [] as an array vs as a method' do
      Parser.parse('foo[1]').should == s(:block, s(:call, s(:call, nil, :foo), :[], s(:lit, 1)))
      Parser.parse('foo [1]').should == s(:block, s(:call, nil, :foo, s(:array, s(:lit, 1))))
      Parser.parse('foo = []; foo[1]').should ==
        s(:block, s(:lasgn, :foo, s(:array)), s(:call, s(:lvar, :foo), :[], s(:lit, 1)))
      Parser.parse('foo = [1]; foo[1]').should ==
        s(:block, s(:lasgn, :foo, s(:array, s(:lit, 1))), s(:call, s(:lvar, :foo), :[], s(:lit, 1)))
      Parser.parse('foo = []; foo [1]').should ==
        s(:block, s(:lasgn, :foo, s(:array)), s(:call, s(:lvar, :foo), :[], s(:lit, 1)))
      Parser.parse('foo = []; foo [1, 2]').should ==
        s(:block, s(:lasgn, :foo, s(:array)), s(:call, s(:lvar, :foo), :[], s(:lit, 1), s(:lit, 2)))
      Parser.parse('foo[a]').should == s(:block, s(:call, s(:call, nil, :foo), :[], s(:call, nil, :a)))
      Parser.parse('foo[]').should == s(:block, s(:call, s(:call, nil, :foo), :[]))
      Parser.parse('foo []').should == s(:block, s(:call, nil, :foo, s(:array)))
      Parser.parse('foo [a, b]').should ==
        s(:block, s(:call, nil, :foo, s(:array, s(:call, nil, :a), s(:call, nil, :b))))
    end

    it 'parses method definition' do
      Parser.parse("def foo\nend").should == s(:block, s(:defn, :foo, s(:args), s(:nil)))
      Parser.parse('def foo;end').should == s(:block, s(:defn, :foo, s(:args), s(:nil)))
      Parser.parse("def foo\n1\nend").should == s(:block, s(:defn, :foo, s(:args), s(:lit, 1)))
      Parser.parse('def foo;1;end').should == s(:block, s(:defn, :foo, s(:args), s(:lit, 1)))
      Parser.parse('def foo();1;end').should == s(:block, s(:defn, :foo, s(:args), s(:lit, 1)))
      Parser.parse('def foo() 1 end').should == s(:block, s(:defn, :foo, s(:args), s(:lit, 1)))
      Parser.parse("def foo;1;2 + 2;'foo';end").should ==
        s(:block, s(:defn, :foo, s(:args), s(:lit, 1), s(:call, s(:lit, 2), :+, s(:lit, 2)), s(:str, 'foo')))
      Parser.parse("def foo x, y\nend").should == s(:block, s(:defn, :foo, s(:args, :x, :y), s(:nil)))
      Parser.parse("def foo x,\ny\nend").should == s(:block, s(:defn, :foo, s(:args, :x, :y), s(:nil)))
      Parser.parse("def foo(x, y)\nend").should == s(:block, s(:defn, :foo, s(:args, :x, :y), s(:nil)))
      Parser.parse('def foo(x, y);end').should == s(:block, s(:defn, :foo, s(:args, :x, :y), s(:nil)))
      Parser.parse("def foo(\nx,\n y\n)\nend").should == s(:block, s(:defn, :foo, s(:args, :x, :y), s(:nil)))
      Parser.parse("def foo(x, y)\n1\n2\nend").should ==
        s(:block, s(:defn, :foo, s(:args, :x, :y), s(:lit, 1), s(:lit, 2)))
      Parser.parse("def foo n\nn\nend").should == s(:block, s(:defn, :foo, s(:args, :n), s(:lvar, :n)))
      Parser.parse("def foo((a, b), c, (d, e))\nend").should ==
        s(:block, s(:defn, :foo, s(:args, s(:masgn, :a, :b), :c, s(:masgn, :d, :e)), s(:nil)))
      Parser.parse('def foo!; end').should == s(:block, s(:defn, :foo!, s(:args), s(:nil)))
      Parser.parse('def foo?; end').should == s(:block, s(:defn, :foo?, s(:args), s(:nil)))
      Parser.parse('def foo=; end').should == s(:block, s(:defn, :foo=, s(:args), s(:nil)))
      Parser.parse('def self.foo=; end').should == s(:block, s(:defs, s(:self), :foo=, s(:args), s(:nil)))
      Parser.parse('def foo.bar=; end').should == s(:block, s(:defs, s(:call, nil, :foo), :bar=, s(:args), s(:nil)))
      Parser.parse('def Foo.foo; end').should == s(:block, s(:defs, s(:const, :Foo), :foo, s(:args), s(:nil)))
      Parser.parse('foo=o; def foo.bar; end').should ==
        s(:block, s(:lasgn, :foo, s(:call, nil, :o)), s(:defs, s(:lvar, :foo), :bar, s(:args), s(:nil)))
      Parser.parse('def foo(*); end').should == s(:block, s(:defn, :foo, s(:args, :*), s(:nil)))
      Parser.parse('def foo(*x); end').should == s(:block, s(:defn, :foo, s(:args, :'*x'), s(:nil)))
      Parser.parse('def foo(x, *y, z); end').should == s(:block, s(:defn, :foo, s(:args, :x, :'*y', :z), s(:nil)))
      Parser.parse('def foo(a, &b); end').should == s(:block, s(:defn, :foo, s(:args, :a, :'&b'), s(:nil)))
      Parser.parse('def foo(a = nil, b = foo, c = FOO); end').should ==
        s(
          :block,
          s(
            :defn,
            :foo,
            s(:args, s(:lasgn, :a, s(:nil)), s(:lasgn, :b, s(:call, nil, :foo)), s(:lasgn, :c, s(:const, :FOO))),
            s(:nil),
          ),
        )
      Parser.parse('def foo(a, b: :c, d:); end').should ==
        s(:block, s(:defn, :foo, s(:args, :a, s(:kwarg, :b, s(:lit, :c)), s(:kwarg, :d)), s(:nil)))
    end

    it 'parses operator method definitions' do
      operators = %w[+ - * ** / % == === != =~ !~ > >= < <= <=> & | ^ ~ << >> [] []=]
      operators.each do |operator|
        Parser.parse("def #{operator}; end").should == s(:block, s(:defn, operator.to_sym, s(:args), s(:nil)))
        Parser.parse("def self.#{operator}; end").should ==
          s(:block, s(:defs, s(:self), operator.to_sym, s(:args), s(:nil)))
      end
    end

    it 'parses method calls vs local variable lookup' do
      Parser.parse('foo').should == s(:block, s(:call, nil, :foo))
      Parser.parse('foo?').should == s(:block, s(:call, nil, :foo?))
      Parser.parse('foo = 1; foo').should == s(:block, s(:lasgn, :foo, s(:lit, 1)), s(:lvar, :foo))
      Parser.parse('foo = 1; def bar; foo; end').should ==
        s(:block, s(:lasgn, :foo, s(:lit, 1)), s(:defn, :bar, s(:args), s(:call, nil, :foo)))
      Parser.parse('@foo = 1; foo').should == s(:block, s(:iasgn, :@foo, s(:lit, 1)), s(:call, nil, :foo))
      Parser.parse('foo, bar = [1, 2]; foo; bar').should ==
        s(
          :block,
          s(:masgn, s(:array, s(:lasgn, :foo), s(:lasgn, :bar)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2)))),
          s(:lvar, :foo),
          s(:lvar, :bar),
        )
      Parser.parse('(foo, (bar, baz)) = [1, [2, 3]]; foo; bar; baz').should ==
        s(
          :block,
          s(
            :masgn,
            s(:array, s(:lasgn, :foo), s(:masgn, s(:array, s(:lasgn, :bar), s(:lasgn, :baz)))),
            s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3)))),
          ),
          s(:lvar, :foo),
          s(:lvar, :bar),
          s(:lvar, :baz),
        )
    end

    it 'parses constants' do
      Parser.parse('ARGV').should == s(:block, s(:const, :ARGV))
      Parser.parse('Foo::Bar').should == s(:block, s(:colon2, s(:const, :Foo), :Bar))
      Parser.parse('Foo::Bar::BAZ').should == s(:block, s(:colon2, s(:colon2, s(:const, :Foo), :Bar), :BAZ))
      Parser.parse('x, y = ::Bar').should ==
        s(:block, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:colon3, :Bar))))
      Parser.parse('Foo::bar').should == s(:block, s(:call, s(:const, :Foo), :bar))
      Parser.parse('Foo::bar = 1 + 2').should ==
        s(:block, s(:attrasgn, s(:const, :Foo), :bar=, s(:call, s(:lit, 1), :+, s(:lit, 2))))
      Parser.parse('Foo::bar x, y').should ==
        s(:block, s(:call, s(:const, :Foo), :bar, s(:call, nil, :x), s(:call, nil, :y)))
    end

    it 'parses global variables' do
      Parser.parse('$foo').should == s(:block, s(:gvar, :$foo))
      Parser.parse('$0').should == s(:block, s(:gvar, :$0))
    end

    it 'parses regexp nth refs' do
      Parser.parse('$1').should == s(:block, s(:nth_ref, 1))
      Parser.parse('$9').should == s(:block, s(:nth_ref, 9))
      Parser.parse('$10').should == s(:block, s(:nth_ref, 10))
      Parser.parse('$100').should == s(:block, s(:nth_ref, 100))
    end

    it 'parses instance variables' do
      Parser.parse('@foo').should == s(:block, s(:ivar, :@foo))
    end

    it 'parses class variables' do
      Parser.parse('@@foo').should == s(:block, s(:cvar, :@@foo))
    end

    it 'parses method calls with parentheses' do
      Parser.parse('foo()').should == s(:block, s(:call, nil, :foo))
      Parser.parse('foo() + bar()').should == s(:block, s(:call, s(:call, nil, :foo), :+, s(:call, nil, :bar)))
      Parser.parse("foo(1, 'baz')").should == s(:block, s(:call, nil, :foo, s(:lit, 1), s(:str, 'baz')))
      Parser.parse('foo(a, b)').should == s(:block, s(:call, nil, :foo, s(:call, nil, :a), s(:call, nil, :b)))
      Parser.parse("foo(\n1 + 2  ,\n  'baz'  \n )").should ==
        s(:block, s(:call, nil, :foo, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, 'baz')))
      Parser.parse('foo(1, a: 2)').should ==
        s(:block, s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2))))
      Parser.parse('foo(1, { a: 2 })').should ==
        s(:block, s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2))))
      Parser.parse('foo(1, { a: 2, :b => 3 })').should ==
        s(:block, s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2), s(:lit, :b), s(:lit, 3))))
      Parser.parse('foo(:a, :b)').should == s(:block, s(:call, nil, :foo, s(:lit, :a), s(:lit, :b)))
      Parser.parse('foo(a: 1)').should == s(:block, s(:call, nil, :foo, s(:hash, s(:lit, :a), s(:lit, 1))))
      Parser.parse("foo(0, a: 1, b: 'two')").should ==
        s(:block, s(:call, nil, :foo, s(:lit, 0), s(:hash, s(:lit, :a), s(:lit, 1), s(:lit, :b), s(:str, 'two'))))
      Parser.parse('foo(a, *b, c)').should ==
        s(:block, s(:call, nil, :foo, s(:call, nil, :a), s(:splat, s(:call, nil, :b)), s(:call, nil, :c)))
      Parser.parse('b=1; foo(a, *b, c)').should ==
        s(
          :block,
          s(:lasgn, :b, s(:lit, 1)),
          s(:call, nil, :foo, s(:call, nil, :a), s(:splat, s(:lvar, :b)), s(:call, nil, :c)),
        )
      Parser.parse('foo.()').should == s(:block, s(:call, s(:call, nil, :foo), :call))
      Parser.parse('foo.(1, 2)').should == s(:block, s(:call, s(:call, nil, :foo), :call, s(:lit, 1), s(:lit, 2)))
      if (RUBY_ENGINE == 'natalie')
        -> { Parser.parse('foo(') }.should raise_error(
                                             SyntaxError,
                                             "(string)#1: syntax error, unexpected end-of-input (expected: 'expression')",
                                           )
      else
        -> { Parser.parse('foo(') }.should raise_error(SyntaxError, '(string):1 :: parse error on value false ($end)')
      end
    end

    it 'parses method calls without parentheses' do
      Parser.parse('foo + bar').should == s(:block, s(:call, s(:call, nil, :foo), :+, s(:call, nil, :bar)))
      Parser.parse("foo 1, 'baz'").should == s(:block, s(:call, nil, :foo, s(:lit, 1), s(:str, 'baz')))
      Parser.parse('foo 1 + 2').should == s(:block, s(:call, nil, :foo, s(:call, s(:lit, 1), :+, s(:lit, 2))))
      Parser.parse("foo 1 + 2  ,\n  'baz'").should ==
        s(:block, s(:call, nil, :foo, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, 'baz')))
      Parser.parse("foo 'foo' + 'bar'  ,\n  2").should ==
        s(:block, s(:call, nil, :foo, s(:call, s(:str, 'foo'), :+, s(:str, 'bar')), s(:lit, 2)))
      Parser.parse('foo a, b').should == s(:block, s(:call, nil, :foo, s(:call, nil, :a), s(:call, nil, :b)))
      Parser.parse('foo 1, a: 2').should ==
        s(:block, s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2))))
      Parser.parse('foo 1, :a => 2, b: 3').should ==
        s(:block, s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2), s(:lit, :b), s(:lit, 3))))
      Parser.parse('foo 1, { a: 2 }').should ==
        s(:block, s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2))))
      Parser.parse('foo 1, { a: 2 }').should ==
        s(:block, s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2))))
      Parser.parse('foo a: 1').should == s(:block, s(:call, nil, :foo, s(:hash, s(:lit, :a), s(:lit, 1))))
      Parser.parse("foo 0, a: 1, b: 'two'").should ==
        s(:block, s(:call, nil, :foo, s(:lit, 0), s(:hash, s(:lit, :a), s(:lit, 1), s(:lit, :b), s(:str, 'two'))))
      Parser.parse('foo :a, :b').should == s(:block, s(:call, nil, :foo, s(:lit, :a), s(:lit, :b)))
      Parser.parse('self.class').should == s(:block, s(:call, s(:self), :class))
      Parser.parse('self.begin').should == s(:block, s(:call, s(:self), :begin))
      Parser.parse('self.end').should == s(:block, s(:call, s(:self), :end))
    end

    it 'parses operator method calls' do
      operators = %w[+ - * ** / % == === != =~ !~ > >= < <= <=> & | ^ ~ << >> [] []=]
      operators.each do |operator|
        Parser.parse("self.#{operator}").should == s(:block, s(:call, s(:self), operator.to_sym))
      end
    end

    it 'parses method calls with a receiver' do
      Parser.parse('foo.bar').should == s(:block, s(:call, s(:call, nil, :foo), :bar))
      Parser.parse('foo.bar.baz').should == s(:block, s(:call, s(:call, s(:call, nil, :foo), :bar), :baz))
      Parser.parse('foo.bar 1, 2').should == s(:block, s(:call, s(:call, nil, :foo), :bar, s(:lit, 1), s(:lit, 2)))
      Parser.parse('foo.bar(1, 2)').should == s(:block, s(:call, s(:call, nil, :foo), :bar, s(:lit, 1), s(:lit, 2)))
      Parser.parse('foo.nil?').should == s(:block, s(:call, s(:call, nil, :foo), :nil?))
      Parser.parse('foo.not?').should == s(:block, s(:call, s(:call, nil, :foo), :not?))
      Parser.parse('foo.baz?').should == s(:block, s(:call, s(:call, nil, :foo), :baz?))
      Parser.parse("foo\n  .bar\n  .baz").should == s(:block, s(:call, s(:call, s(:call, nil, :foo), :bar), :baz))
      Parser.parse("foo.\n  bar.\n  baz").should == s(:block, s(:call, s(:call, s(:call, nil, :foo), :bar), :baz))
    end

    it 'parses ternary expressions' do
      Parser.parse('1 ? 2 : 3').should == s(:block, s(:if, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
      Parser.parse("foo ?\nbar + baz\n :\n buz / 2").should ==
        s(
          :block,
          s(
            :if,
            s(:call, nil, :foo),
            s(:call, s(:call, nil, :bar), :+, s(:call, nil, :baz)),
            s(:call, s(:call, nil, :buz), :/, s(:lit, 2)),
          ),
        )
      Parser.parse('1 ? 2 : map { |n| n }').should ==
        s(:block, s(:if, s(:lit, 1), s(:lit, 2), s(:iter, s(:call, nil, :map), s(:args, :n), s(:lvar, :n))))
      Parser.parse("1 ? 2 : map do |n|\nn\nend").should ==
        s(:block, s(:if, s(:lit, 1), s(:lit, 2), s(:iter, s(:call, nil, :map), s(:args, :n), s(:lvar, :n))))
      Parser.parse('fib(num ? num.to_i : 25)').should ==
        s(:block, s(:call, nil, :fib, s(:if, s(:call, nil, :num), s(:call, s(:call, nil, :num), :to_i), s(:lit, 25))))
    end

    it 'parses if/elsif/else' do
      Parser.parse('if true; 1; end').should == s(:block, s(:if, s(:true), s(:lit, 1), nil))
      Parser.parse('if true; 1; 2; end').should == s(:block, s(:if, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), nil))
      Parser.parse('if false; 1; else; 2; end').should == s(:block, s(:if, s(:false), s(:lit, 1), s(:lit, 2)))
      Parser.parse('if false; 1; elsif 1 + 1 == 2; 2; else; 3; end').should ==
        s(
          :block,
          s(
            :if,
            s(:false),
            s(:lit, 1),
            s(:if, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 1)), :==, s(:lit, 2)), s(:lit, 2), s(:lit, 3)),
          ),
        )
      Parser.parse("if false; 1; elsif 1 + 1 == 0; 2; 3; elsif false; 4; elsif foo() == 'bar'; 5; 6; else; 7; end")
        .should ==
        s(
          :block,
          s(
            :if,
            s(:false),
            s(:lit, 1),
            s(
              :if,
              s(:call, s(:call, s(:lit, 1), :+, s(:lit, 1)), :==, s(:lit, 0)),
              s(:block, s(:lit, 2), s(:lit, 3)),
              s(
                :if,
                s(:false),
                s(:lit, 4),
                s(
                  :if,
                  s(:call, s(:call, nil, :foo), :==, s(:str, 'bar')),
                  s(:block, s(:lit, 5), s(:lit, 6)),
                  s(:lit, 7),
                ),
              ),
            ),
          ),
        )
    end

    it 'parses unless' do
      Parser.parse('unless false; 1; else; 2; end').should == s(:block, s(:if, s(:false), s(:lit, 2), s(:lit, 1)))
    end

    it 'parses while/until' do
      Parser.parse('while true; end').should == s(:block, s(:while, s(:true), nil, true))
      Parser.parse('while true; 1; end').should == s(:block, s(:while, s(:true), s(:lit, 1), true))
      Parser.parse('while true; 1; 2; end').should ==
        s(:block, s(:while, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), true))
      Parser.parse('until true; end').should == s(:block, s(:until, s(:true), nil, true))
      Parser.parse('until true; 1; end').should == s(:block, s(:until, s(:true), s(:lit, 1), true))
      Parser.parse('until true; 1; 2; end').should ==
        s(:block, s(:until, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), true))
      Parser.parse('begin; 1; end while true').should == s(:block, s(:while, s(:true), s(:lit, 1), false))
      Parser.parse('begin; 1; 2; end while true').should ==
        s(:block, s(:while, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), false))
      Parser.parse('begin; 1; rescue; 2; end while true').should ==
        s(:block, s(:while, s(:true), s(:rescue, s(:lit, 1), s(:resbody, s(:array), s(:lit, 2))), false))
      Parser.parse('begin; 1; ensure; 2; end while true').should ==
        s(:block, s(:while, s(:true), s(:ensure, s(:lit, 1), s(:lit, 2)), false))
      Parser.parse('begin; 1; end until true').should == s(:block, s(:until, s(:true), s(:lit, 1), false))
      Parser.parse('begin; 1; 2; end until true').should ==
        s(:block, s(:until, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), false))
      Parser.parse('begin; 1; rescue; 2; end until true').should ==
        s(:block, s(:until, s(:true), s(:rescue, s(:lit, 1), s(:resbody, s(:array), s(:lit, 2))), false))
      Parser.parse('begin; 1; ensure; 2; end until true').should ==
        s(:block, s(:until, s(:true), s(:ensure, s(:lit, 1), s(:lit, 2)), false))
    end

    it 'parses post-conditional if/unless' do
      Parser.parse('true if true').should == s(:block, s(:if, s(:true), s(:true), nil))
      Parser.parse('true unless true').should == s(:block, s(:if, s(:true), nil, s(:true)))
      Parser.parse("foo 'hi' if true").should == s(:block, s(:if, s(:true), s(:call, nil, :foo, s(:str, 'hi')), nil))
      Parser.parse("foo.bar 'hi' if true").should ==
        s(:block, s(:if, s(:true), s(:call, s(:call, nil, :foo), :bar, s(:str, 'hi')), nil))
    end

    it 'parses true/false/nil' do
      Parser.parse('true').should == s(:block, s(:true))
      Parser.parse('false').should == s(:block, s(:false))
      Parser.parse('nil').should == s(:block, s(:nil))
    end

    it 'parses examples/fib.rb' do
      fib = File.read(File.expand_path('../../examples/fib.rb', __dir__))
      Parser.parse(fib).should ==
        s(
          :block,
          s(
            :defn,
            :fib,
            s(:args, :n),
            s(
              :if,
              s(:call, s(:lvar, :n), :==, s(:lit, 0)),
              s(:lit, 0),
              s(
                :if,
                s(:call, s(:lvar, :n), :==, s(:lit, 1)),
                s(:lit, 1),
                s(
                  :call,
                  s(:call, nil, :fib, s(:call, s(:lvar, :n), :-, s(:lit, 1))),
                  :+,
                  s(:call, nil, :fib, s(:call, s(:lvar, :n), :-, s(:lit, 2))),
                ),
              ),
            ),
          ),
          s(:lasgn, :num, s(:call, s(:const, :ARGV), :first)),
          s(
            :call,
            nil,
            :puts,
            s(:call, nil, :fib, s(:if, s(:lvar, :num), s(:call, s(:lvar, :num), :to_i), s(:lit, 25))),
          ),
        )
    end

    it 'parses class definition' do
      Parser.parse("class Foo\nend").should == s(:block, s(:class, :Foo, nil))
      Parser.parse('class Foo;end').should == s(:block, s(:class, :Foo, nil))
      Parser.parse('class FooBar; 1; 2; end').should == s(:block, s(:class, :FooBar, nil, s(:lit, 1), s(:lit, 2)))
      -> { Parser.parse('class foo;end') }.should raise_error(SyntaxError, 'class/module name must be CONSTANT')
      Parser.parse("class Foo < Bar; 3\n 4\n end").should ==
        s(:block, s(:class, :Foo, s(:const, :Bar), s(:lit, 3), s(:lit, 4)))
      Parser.parse("class Foo < bar; 3\n 4\n end").should ==
        s(:block, s(:class, :Foo, s(:call, nil, :bar), s(:lit, 3), s(:lit, 4)))
    end

    it 'parses class << self' do
      Parser.parse('class Foo; class << self; end; end').should == s(:block, s(:class, :Foo, nil, s(:sclass, s(:self))))
      Parser.parse('class Foo; class << Bar; 1; end; end').should ==
        s(:block, s(:class, :Foo, nil, s(:sclass, s(:const, :Bar), s(:lit, 1))))
      Parser.parse('class Foo; class << (1 + 1); 1; 2; end; end').should ==
        s(:block, s(:class, :Foo, nil, s(:sclass, s(:call, s(:lit, 1), :+, s(:lit, 1)), s(:lit, 1), s(:lit, 2))))
    end

    it 'parses module definition' do
      Parser.parse("module Foo\nend").should == s(:block, s(:module, :Foo))
      Parser.parse('module Foo;end').should == s(:block, s(:module, :Foo))
      Parser.parse('module FooBar; 1; 2; end').should == s(:block, s(:module, :FooBar, s(:lit, 1), s(:lit, 2)))
      -> { Parser.parse('module foo;end') }.should raise_error(SyntaxError, 'class/module name must be CONSTANT')
    end

    it 'parses array' do
      Parser.parse('[]').should == s(:block, s(:array))
      Parser.parse('[1]').should == s(:block, s(:array, s(:lit, 1)))
      Parser.parse('[1,]').should == s(:block, s(:array, s(:lit, 1)))
      Parser.parse("['foo']").should == s(:block, s(:array, s(:str, 'foo')))
      Parser.parse('[1, 2, 3]').should == s(:block, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
      Parser.parse('[1, 2, 3, ]').should == s(:block, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
      Parser.parse('[x, y, z]').should == s(:block, s(:array, s(:call, nil, :x), s(:call, nil, :y), s(:call, nil, :z)))
      Parser.parse("[\n1 , \n2,\n 3]").should == s(:block, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
      Parser.parse("[\n1 , \n2,\n 3\n]").should == s(:block, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
      Parser.parse("[\n1 , \n2,\n 3,\n]").should == s(:block, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
      if (RUBY_ENGINE == 'natalie')
        -> { Parser.parse('[ , 1]') }.should raise_error(
                                               SyntaxError,
                                               "(string)#1: syntax error, unexpected ',' (expected: 'expression')",
                                             )
      else
        -> { Parser.parse('[ , 1]') }.should raise_error(SyntaxError, '(string):1 :: parse error on value "," (tCOMMA)')
      end
    end

    it 'parses word array' do
      Parser.parse('%w[]').should == s(:block, s(:array))
      Parser.parse('%w|1 2 3|').should == s(:block, s(:array, s(:str, '1'), s(:str, '2'), s(:str, '3')))
      Parser.parse("%w[  1 2\t  3\n \n4 ]").should ==
        s(:block, s(:array, s(:str, '1'), s(:str, '2'), s(:str, '3'), s(:str, '4')))
      Parser.parse("%W[  1 2\t  3\n \n4 ]").should ==
        s(:block, s(:array, s(:str, '1'), s(:str, '2'), s(:str, '3'), s(:str, '4')))
      Parser.parse('%i[ foo bar ]').should == s(:block, s(:array, s(:lit, :foo), s(:lit, :bar)))
      Parser.parse('%I[ foo bar ]').should == s(:block, s(:array, s(:lit, :foo), s(:lit, :bar)))
    end

    it 'parses hash' do
      Parser.parse('{}').should == s(:block, s(:hash))
      Parser.parse('{ 1 => 2 }').should == s(:block, s(:hash, s(:lit, 1), s(:lit, 2)))
      Parser.parse('{ 1 => 2, }').should == s(:block, s(:hash, s(:lit, 1), s(:lit, 2)))
      Parser.parse("{\n 1 => 2,\n }").should == s(:block, s(:hash, s(:lit, 1), s(:lit, 2)))
      Parser.parse("{ foo: 'bar' }").should == s(:block, s(:hash, s(:lit, :foo), s(:str, 'bar')))
      Parser.parse("{ 1 => 2, 'foo' => 'bar' }").should ==
        s(:block, s(:hash, s(:lit, 1), s(:lit, 2), s(:str, 'foo'), s(:str, 'bar')))
      Parser.parse("{\n 1 => \n2,\n 'foo' =>\n'bar'\n}").should ==
        s(:block, s(:hash, s(:lit, 1), s(:lit, 2), s(:str, 'foo'), s(:str, 'bar')))
      Parser.parse("{ foo: 'bar', baz: 'buz' }").should ==
        s(:block, s(:hash, s(:lit, :foo), s(:str, 'bar'), s(:lit, :baz), s(:str, 'buz')))
      Parser.parse('{ a => b, c => d }').should ==
        s(:block, s(:hash, s(:call, nil, :a), s(:call, nil, :b), s(:call, nil, :c), s(:call, nil, :d)))
      if (RUBY_ENGINE == 'natalie')
        -> { Parser.parse('{ , 1 => 2 }') }.should raise_error(
                                                     SyntaxError,
                                                     "(string)#1: syntax error, unexpected ',' (expected: 'expression')",
                                                   )
      else
        -> { Parser.parse('{ , 1 => 2 }') }.should raise_error(
                                                     SyntaxError,
                                                     '(string):1 :: parse error on value "," (tCOMMA)',
                                                   )
      end
    end

    it 'ignores comments' do
      Parser.parse('# comment').should == s(:block)
      Parser.parse("# comment\n#comment 2").should == s(:block)
      Parser.parse('1 + 1 # comment').should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 1)))
    end

    it 'parses range' do
      Parser.parse('1..10').should == s(:block, s(:lit, 1..10))
      Parser.parse("1..'a'").should == s(:block, s(:dot2, s(:lit, 1), s(:str, 'a')))
      Parser.parse("'a'..1").should == s(:block, s(:dot2, s(:str, 'a'), s(:lit, 1)))
      Parser.parse('1...10').should == s(:block, s(:lit, 1...10))
      Parser.parse('1.1..2.2').should == s(:block, s(:dot2, s(:lit, 1.1), s(:lit, 2.2)))
      Parser.parse('1.1...2.2').should == s(:block, s(:dot3, s(:lit, 1.1), s(:lit, 2.2)))
      Parser.parse("'a'..'z'").should == s(:block, s(:dot2, s(:str, 'a'), s(:str, 'z')))
      Parser.parse("'a'...'z'").should == s(:block, s(:dot3, s(:str, 'a'), s(:str, 'z')))
      Parser.parse("('a')..('z')").should == s(:block, s(:dot2, s(:str, 'a'), s(:str, 'z')))
      Parser.parse("('a')...('z')").should == s(:block, s(:dot3, s(:str, 'a'), s(:str, 'z')))
      Parser.parse('foo..bar').should == s(:block, s(:dot2, s(:call, nil, :foo), s(:call, nil, :bar)))
      Parser.parse('foo...bar').should == s(:block, s(:dot3, s(:call, nil, :foo), s(:call, nil, :bar)))
      Parser.parse('foo = 1..10').should == s(:block, s(:lasgn, :foo, s(:lit, 1..10)))
    end

    it 'parses return' do
      Parser.parse('return').should == s(:block, s(:return))
      Parser.parse('return foo').should == s(:block, s(:return, s(:call, nil, :foo)))
      Parser.parse('return foo if true').should == s(:block, s(:if, s(:true), s(:return, s(:call, nil, :foo)), nil))
    end

    it 'parses block' do
      Parser.parse("foo do\nend").should == s(:block, s(:iter, s(:call, nil, :foo), 0))
      Parser.parse("foo do\n1\n2\nend").should ==
        s(:block, s(:iter, s(:call, nil, :foo), 0, s(:block, s(:lit, 1), s(:lit, 2))))
      Parser.parse("foo do |x, y|\nx\ny\nend").should ==
        s(:block, s(:iter, s(:call, nil, :foo), s(:args, :x, :y), s(:block, s(:lvar, :x), s(:lvar, :y))))
      Parser.parse("foo(a, b) do |x, y|\nx\ny\nend").should ==
        s(
          :block,
          s(
            :iter,
            s(:call, nil, :foo, s(:call, nil, :a), s(:call, nil, :b)),
            s(:args, :x, :y),
            s(:block, s(:lvar, :x), s(:lvar, :y)),
          ),
        )
      Parser.parse('foo { }').should == s(:block, s(:iter, s(:call, nil, :foo), 0))
      Parser.parse('foo { 1; 2 }').should ==
        s(:block, s(:iter, s(:call, nil, :foo), 0, s(:block, s(:lit, 1), s(:lit, 2))))
      Parser.parse('foo { |x, y| x; y }').should ==
        s(:block, s(:iter, s(:call, nil, :foo), s(:args, :x, :y), s(:block, s(:lvar, :x), s(:lvar, :y))))
      Parser.parse('foo { |x| x }; x').should ==
        s(:block, s(:iter, s(:call, nil, :foo), s(:args, :x), s(:lvar, :x)), s(:call, nil, :x))
      Parser.parse('x = 1; foo { x }; x').should ==
        s(:block, s(:lasgn, :x, s(:lit, 1)), s(:iter, s(:call, nil, :foo), 0, s(:lvar, :x)), s(:lvar, :x))
      Parser.parse("foo do\nbar do\nend\nend").should ==
        s(:block, s(:iter, s(:call, nil, :foo), 0, s(:iter, s(:call, nil, :bar), 0)))
      Parser.parse("foo do |(x, y), z|\nend").should ==
        s(:block, s(:iter, s(:call, nil, :foo), s(:args, s(:masgn, :x, :y), :z)))
      Parser.parse("foo do |(x, (y, z))|\nend").should ==
        s(:block, s(:iter, s(:call, nil, :foo), s(:args, s(:masgn, :x, s(:masgn, :y, :z)))))
      Parser.parse('x = foo.bar { |y| y }').should ==
        s(:block, s(:lasgn, :x, s(:iter, s(:call, s(:call, nil, :foo), :bar), s(:args, :y), s(:lvar, :y))))
      Parser.parse('bar { |*| 1 }').should == s(:block, s(:iter, s(:call, nil, :bar), s(:args, :*), s(:lit, 1)))
      Parser.parse('bar { |*x| x }').should == s(:block, s(:iter, s(:call, nil, :bar), s(:args, :'*x'), s(:lvar, :x)))
      Parser.parse('bar { |x, *y, z| y }').should ==
        s(:block, s(:iter, s(:call, nil, :bar), s(:args, :x, :'*y', :z), s(:lvar, :y)))
      Parser.parse('bar { |a = nil, b = foo, c = FOO| b }').should ==
        s(
          :block,
          s(
            :iter,
            s(:call, nil, :bar),
            s(:args, s(:lasgn, :a, s(:nil)), s(:lasgn, :b, s(:call, nil, :foo)), s(:lasgn, :c, s(:const, :FOO))),
            s(:lvar, :b),
          ),
        )
      Parser.parse('bar { |a, b: :c, d:| a }').should ==
        s(:block, s(:iter, s(:call, nil, :bar), s(:args, :a, s(:kwarg, :b, s(:lit, :c)), s(:kwarg, :d)), s(:lvar, :a)))
      Parser.parse("get 'foo', bar { 'baz' }").should ==
        s(:block, s(:call, nil, :get, s(:str, 'foo'), s(:iter, s(:call, nil, :bar), 0, s(:str, 'baz'))))
      Parser.parse("get 'foo', bar do\n'baz'\nend").should ==
        s(:block, s(:iter, s(:call, nil, :get, s(:str, 'foo'), s(:call, nil, :bar)), 0, s(:str, 'baz')))
      Parser.parse("foo1 = foo do\n'foo'\nend").should ==
        s(:block, s(:lasgn, :foo1, s(:iter, s(:call, nil, :foo), 0, s(:str, 'foo'))))
      Parser.parse("@foo = foo do\n'foo'\nend").should ==
        s(:block, s(:iasgn, :@foo, s(:iter, s(:call, nil, :foo), 0, s(:str, 'foo'))))
      Parser.parse("@foo ||= foo do\n'foo'\nend").should ==
        s(:block, s(:op_asgn_or, s(:ivar, :@foo), s(:iasgn, :@foo, s(:iter, s(:call, nil, :foo), 0, s(:str, 'foo')))))
      Parser.parse("get 'foo', bar do 'baz'\n end").should ==
        s(:block, s(:iter, s(:call, nil, :get, s(:str, 'foo'), s(:call, nil, :bar)), 0, s(:str, 'baz')))
      Parser.parse("get 'foo', bar do\n 'baz' end").should ==
        s(:block, s(:iter, s(:call, nil, :get, s(:str, 'foo'), s(:call, nil, :bar)), 0, s(:str, 'baz')))
      Parser.parse("get 'foo', bar do 'baz' end").should ==
        s(:block, s(:iter, s(:call, nil, :get, s(:str, 'foo'), s(:call, nil, :bar)), 0, s(:str, 'baz')))
    end

    it 'parses block pass (ampersand operator)' do
      Parser.parse('map(&:foo)').should == s(:block, s(:call, nil, :map, s(:block_pass, s(:lit, :foo))))
      Parser.parse('map(&myblock)').should == s(:block, s(:call, nil, :map, s(:block_pass, s(:call, nil, :myblock))))
      Parser.parse('map(&nil)').should == s(:block, s(:call, nil, :map, s(:block_pass, s(:nil))))
    end

    it 'parses break, next, super, and yield' do
      Parser.parse('break').should == s(:block, s(:break))
      Parser.parse('break()').should == s(:block, s(:break, s(:nil)))
      Parser.parse('break 1, 2').should == s(:block, s(:break, s(:array, s(:lit, 1), s(:lit, 2))))
      Parser.parse('break([1, 2])').should == s(:block, s(:break, s(:array, s(:lit, 1), s(:lit, 2))))
      Parser.parse('break if true').should == s(:block, s(:if, s(:true), s(:break), nil))
      Parser.parse('next').should == s(:block, s(:next))
      Parser.parse('next()').should == s(:block, s(:next, s(:nil)))
      Parser.parse('next 1, 2').should == s(:block, s(:next, s(:array, s(:lit, 1), s(:lit, 2))))
      Parser.parse('next([1, 2])').should == s(:block, s(:next, s(:array, s(:lit, 1), s(:lit, 2))))
      Parser.parse('next if true').should == s(:block, s(:if, s(:true), s(:next), nil))
      Parser.parse('super').should == s(:block, s(:zsuper))
      Parser.parse('super()').should == s(:block, s(:super))
      Parser.parse('super 1, 2').should == s(:block, s(:super, s(:lit, 1), s(:lit, 2)))
      Parser.parse('super([1, 2])').should == s(:block, s(:super, s(:array, s(:lit, 1), s(:lit, 2))))
      Parser.parse('super if true').should == s(:block, s(:if, s(:true), s(:zsuper), nil))
      Parser.parse('yield').should == s(:block, s(:yield))
      Parser.parse('yield()').should == s(:block, s(:yield))
      Parser.parse('yield 1, 2').should == s(:block, s(:yield, s(:lit, 1), s(:lit, 2)))
      Parser.parse('yield(1, 2)').should == s(:block, s(:yield, s(:lit, 1), s(:lit, 2)))
      Parser.parse('yield if true').should == s(:block, s(:if, s(:true), s(:yield), nil))
    end

    it 'parses self' do
      Parser.parse('self').should == s(:block, s(:self))
    end

    it 'parses __FILE__ and __dir__' do
      Parser.parse('__FILE__', 'foo/bar.rb').should == s(:block, s(:str, 'foo/bar.rb'))
      Parser.parse('__dir__').should == s(:block, s(:call, nil, :__dir__))
    end

    it 'parses splat *' do
      Parser.parse('def foo(*args); end').should == s(:block, s(:defn, :foo, s(:args, :'*args'), s(:nil)))
      Parser.parse('def foo *args; end').should == s(:block, s(:defn, :foo, s(:args, :'*args'), s(:nil)))
      Parser.parse('foo(*args)').should == s(:block, s(:call, nil, :foo, s(:splat, s(:call, nil, :args))))
    end

    it 'parses keyword splat *' do
      Parser.parse('def foo(**kwargs); end').should == s(:block, s(:defn, :foo, s(:args, :'**kwargs'), s(:nil)))
      Parser.parse('def foo **kwargs; end').should == s(:block, s(:defn, :foo, s(:args, :'**kwargs'), s(:nil)))
      Parser.parse('foo(**kwargs)').should ==
        s(:block, s(:call, nil, :foo, s(:hash, s(:kwsplat, s(:call, nil, :kwargs)))))
    end

    it 'parses stabby proc' do
      Parser.parse('-> { puts 1 }').should == s(:block, s(:iter, s(:lambda), 0, s(:call, nil, :puts, s(:lit, 1))))
      Parser.parse('-> x { puts x }').should ==
        s(:block, s(:iter, s(:lambda), s(:args, :x), s(:call, nil, :puts, s(:lvar, :x))))
      Parser.parse('-> x, y { puts x, y }').should ==
        s(:block, s(:iter, s(:lambda), s(:args, :x, :y), s(:call, nil, :puts, s(:lvar, :x), s(:lvar, :y))))
      Parser.parse('-> (x, y) { x; y }').should ==
        s(:block, s(:iter, s(:lambda), s(:args, :x, :y), s(:block, s(:lvar, :x), s(:lvar, :y))))
      -> { Parser.parse('->') }.should raise_error(SyntaxError)
    end

    it 'parses case/when/else' do
      Parser.parse("case 1\nwhen 1\n:a\nend").should ==
        s(:block, s(:case, s(:lit, 1), s(:when, s(:array, s(:lit, 1)), s(:lit, :a)), nil))
      Parser.parse("case 1\nwhen 1\n:a\n:b\nwhen 2, 3\n:c\nelse\n:d\nend").should ==
        s(
          :block,
          s(
            :case,
            s(:lit, 1),
            s(:when, s(:array, s(:lit, 1)), s(:lit, :a), s(:lit, :b)),
            s(:when, s(:array, s(:lit, 2), s(:lit, 3)), s(:lit, :c)),
            s(:lit, :d),
          ),
        )
    end

    it 'parses begin/rescue/else/ensure' do
      Parser.parse('begin;1;2;rescue;3;4;end').should ==
        s(:block, s(:rescue, s(:block, s(:lit, 1), s(:lit, 2)), s(:resbody, s(:array), s(:lit, 3), s(:lit, 4))))
      Parser.parse('begin;1;rescue => e;e;end').should ==
        s(:block, s(:rescue, s(:lit, 1), s(:resbody, s(:array, s(:lasgn, :e, s(:gvar, :$!))), s(:lvar, :e))))
      Parser.parse('begin;rescue SyntaxError, NoMethodError => e;3;end').should ==
        s(
          :block,
          s(
            :rescue,
            s(
              :resbody,
              s(:array, s(:const, :SyntaxError), s(:const, :NoMethodError), s(:lasgn, :e, s(:gvar, :$!))),
              s(:lit, 3),
            ),
          ),
        )
      Parser.parse('begin;rescue SyntaxError;3;else;4;5;end').should ==
        s(
          :block,
          s(:rescue, s(:resbody, s(:array, s(:const, :SyntaxError)), s(:lit, 3)), s(:block, s(:lit, 4), s(:lit, 5))),
        )
      Parser.parse("begin\n0\nensure\n:a\n:b\nend").should ==
        s(:block, s(:ensure, s(:lit, 0), s(:block, s(:lit, :a), s(:lit, :b))))
      Parser.parse('begin;0;rescue;:a;else;:c;ensure;:d;:e;end').should ==
        s(
          :block,
          s(
            :ensure,
            s(:rescue, s(:lit, 0), s(:resbody, s(:array), s(:lit, :a)), s(:lit, :c)),
            s(:block, s(:lit, :d), s(:lit, :e)),
          ),
        )
      Parser.parse('def foo;0;rescue;:a;else;:c;ensure;:d;:e;end').should ==
        s(
          :block,
          s(
            :defn,
            :foo,
            s(:args),
            s(
              :ensure,
              s(:rescue, s(:lit, 0), s(:resbody, s(:array), s(:lit, :a)), s(:lit, :c)),
              s(:block, s(:lit, :d), s(:lit, :e)),
            ),
          ),
        )
      Parser.parse('begin;0;rescue foo(1), bar(2);1;end').should ==
        s(
          :block,
          s(
            :rescue,
            s(:lit, 0),
            s(:resbody, s(:array, s(:call, nil, :foo, s(:lit, 1)), s(:call, nil, :bar, s(:lit, 2))), s(:lit, 1)),
          ),
        )
      Parser.parse('begin;0;ensure;1;end').should == s(:block, s(:ensure, s(:lit, 0), s(:lit, 1)))
    end

    it 'parses backticks and %x()' do
      Parser.parse('`ls`').should == s(:block, s(:xstr, 'ls'))
      Parser.parse('%x(ls)').should == s(:block, s(:xstr, 'ls'))
      Parser.parse("%x(ls \#{path})").should == s(:block, s(:dxstr, 'ls ', s(:evstr, s(:call, nil, :path))))
    end

    it 'parses alias' do
      Parser.parse('alias foo bar').should == s(:block, s(:alias, s(:lit, :foo), s(:lit, :bar)))
      Parser.parse('alias :foo :bar').should == s(:block, s(:alias, s(:lit, :foo), s(:lit, :bar)))
      Parser.parse("alias write <<\ndef foo; end").should ==
        s(:block, s(:alias, s(:lit, :write), s(:lit, :<<)), s(:defn, :foo, s(:args), s(:nil)))
    end

    it 'parses defined?' do
      Parser.parse('defined? foo').should == s(:block, s(:defined, s(:call, nil, :foo)))
      Parser.parse('defined?(:foo)').should == s(:block, s(:defined, s(:lit, :foo)))
    end

    it 'parses heredocs' do
      doc1 = <<END
foo = <<FOO_BAR
 1
2
FOO_BAR
END
      Parser.parse(doc1).should == s(:block, s(:lasgn, :foo, s(:str, " 1\n2\n")))
      doc2 = <<END
foo(1, <<-foo, 2)
 1
2
  foo
END
      Parser.parse(doc2).should == s(:block, s(:call, nil, :foo, s(:lit, 1), s(:str, " 1\n2\n"), s(:lit, 2)))
      doc3 = <<END
<<FOO
  \#{1+1}
FOO
END
      Parser.parse(doc3).should ==
        s(:block, s(:dstr, '  ', s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), s(:str, "\n")))
    end
  end
end
