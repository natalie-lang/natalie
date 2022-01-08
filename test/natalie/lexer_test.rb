# skip-ruby

require_relative '../spec_helper'

describe 'Parser' do
  describe '#tokens' do
    it 'tokenizes keywords' do
      %w[
        __ENCODING__
        __LINE__
        __FILE__
        BEGIN
        END
        alias
        and
        begin
        break
        case
        class
        def
        defined?
        do
        else
        elsif
        end
        ensure
        false
        for
        if
        in
        module
        next
        nil
        not
        or
        redo
        rescue
        retry
        return
        self
        super
        then
        true
        undef
        unless
        until
        when
        while
        yield
      ].each { |keyword| Parser.tokens(keyword).should == [{ type: keyword.to_sym }] }
      Parser.tokens('defx = 1').should == [
        { type: :name, literal: :defx },
        { type: :'=' },
        { type: :integer, literal: 1 },
      ]
    end

    it 'tokenizes division and regexp' do
      Parser.tokens('1/2').should == [{ type: :integer, literal: 1 }, { type: :'/' }, { type: :integer, literal: 2 }]
      Parser.tokens('1 / 2').should == [{ type: :integer, literal: 1 }, { type: :'/' }, { type: :integer, literal: 2 }]
      Parser.tokens('1 / 2 / 3').should == [
        { type: :integer, literal: 1 },
        { type: :'/' },
        { type: :integer, literal: 2 },
        { type: :'/' },
        { type: :integer, literal: 3 },
      ]
      Parser.tokens('foo / 2').should == [
        { type: :name, literal: :foo },
        { type: :'/' },
        { type: :integer, literal: 2 },
      ]
      Parser.tokens('foo /2/').should == [
        { type: :name, literal: :foo },
        { type: :dregx },
        { type: :string, literal: '2' },
        { type: :dregxend },
      ]
      Parser.tokens('foo/2').should == [{ type: :name, literal: :foo }, { type: :'/' }, { type: :integer, literal: 2 }]
      Parser.tokens('foo( /2/ )').should == [
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :dregx },
        { type: :string, literal: '2' },
        { type: :dregxend },
        { type: :')' },
      ]
      Parser.tokens('foo 1,/2/').should == [
        { type: :name, literal: :foo },
        { type: :integer, literal: 1 },
        { type: :',' },
        { type: :dregx },
        { type: :string, literal: '2' },
        { type: :dregxend },
      ]
    end

    it 'tokenizes regexps' do
      Parser.tokens('//mix').should == [{ type: :dregx }, { type: :dregxend, options: 'mix' }]
      Parser.tokens('/foo/i').should == [
        { type: :dregx },
        { type: :string, literal: 'foo' },
        { type: :dregxend, options: 'i' },
      ]
      Parser.tokens('/foo/').should == [{ type: :dregx }, { type: :string, literal: 'foo' }, { type: :dregxend }]
      Parser.tokens('/\/\*\/\n/').should == [
        { type: :dregx },
        { type: :string, literal: "\\/\\*\\/\\n" },
        { type: :dregxend },
      ]
      Parser.tokens('/foo #{1+1} bar/').should == [
        { type: :dregx },
        { type: :string, literal: 'foo ' },
        { type: :evstr },
        { type: :integer, literal: 1 },
        { type: :integer, literal: 1 },
        { type: :"\n" },
        { type: :evstrend },
        { type: :string, literal: ' bar' },
        { type: :dregxend },
      ]
      Parser.tokens('foo =~ /=$/').should == [
        { type: :name, literal: :foo },
        { type: :'=~' },
        { type: :dregx },
        { type: :string, literal: '=$' },
        { type: :dregxend },
      ]
      Parser.tokens('/^$(.)[.]{1}.*.+.?\^\$\.\(\)\[\]\{\}\w\W\d\D\h\H\s\S\R\*\+\?/').should == [
        { type: :dregx },
        { type: :string, literal: "^$(.)[.]{1}.*.+.?\\^\\$\\.\\(\\)\\[\\]\\{\\}\\w\\W\\d\\D\\h\\H\\s\\S\\R\\*\\+\\?" },
        { type: :dregxend },
      ]
      Parser.tokens("/\\n\\\\n/").should == [
        { type: :dregx },
        { type: :string, literal: "\\n\\\\n" },
        { type: :dregxend },
      ]
      Parser.tokens("/\\&\\a\\b/").should == [
        { type: :dregx },
        { type: :string, literal: "\\&\\a\\b" },
        { type: :dregxend },
      ]
    end

    it 'tokenizes operators' do
      operators = %w[
        +
        +=
        -
        -=
        *
        *=
        **
        **=
        /
        /=
        %
        %=
        =
        ==
        ===
        !=
        =~
        !~
        >
        >=
        <
        <=
        <=>
        &
        |
        ^
        ~
        <<
        >>
        &&
        &&=
        ||
        ||=
        !
        ?
        :
        ::
        ..
        ...
        &.
        |=
        &=
        <<=
        >>=
      ]
      Parser.tokens(operators.join(' ')).should == operators.map { |o| { type: o.to_sym } }
    end

    it 'tokenizes numbers' do
      Parser.tokens('1 123 +1 -456 - 0 100_000_000 0d5 0D6 0o10 0O11 0xff 0XFF 0b110 0B111').should == [
        { type: :integer, literal: 1 },
        { type: :integer, literal: 123 },
        { type: :integer, literal: 1 },
        { type: :integer, literal: -456 },
        { type: :'-' },
        { type: :integer, literal: 0 },
        { type: :integer, literal: 100_000_000 },
        { type: :integer, literal: 5 }, # 0d5
        { type: :integer, literal: 6 }, # 0D6
        { type: :integer, literal: 8 }, # 0o10
        { type: :integer, literal: 9 }, # 0O11
        { type: :integer, literal: 255 }, # 0xff
        { type: :integer, literal: 255 }, # 0XFF
        { type: :integer, literal: 6 }, # 0b110
        { type: :integer, literal: 7 }, # 0B111
      ]
      Parser.tokens('1, (123)').should == [
        { type: :integer, literal: 1 },
        { type: :',' },
        { type: :'(' },
        { type: :integer, literal: 123 },
        { type: :')' },
      ]
      -> { Parser.tokens('1x') }.should raise_error(SyntaxError)
      Parser.tokens('1.234').should == [{ type: :float, literal: 1.234 }]
      Parser.tokens('-1.234').should == [{ type: :float, literal: -1.234 }]
      Parser.tokens('0.1').should == [{ type: :float, literal: 0.1 }]
      Parser.tokens('-0.1').should == [{ type: :float, literal: -0.1 }]
      Parser.tokens('123_456.00').should == [{ type: :float, literal: 123456.0 }]
      -> { Parser.tokens('0.1a') }.should raise_error(SyntaxError, "1: syntax error, unexpected 'a'")
      -> { Parser.tokens('0bb') }.should raise_error(SyntaxError, "1: syntax error, unexpected 'b'")
      -> { Parser.tokens('0dc') }.should raise_error(SyntaxError, "1: syntax error, unexpected 'c'")
      -> { Parser.tokens('0d2d') }.should raise_error(SyntaxError, "1: syntax error, unexpected 'd'")
      -> { Parser.tokens('0o2e') }.should raise_error(SyntaxError, "1: syntax error, unexpected 'e'")
      -> { Parser.tokens('0x2z') }.should raise_error(SyntaxError, "1: syntax error, unexpected 'z'")
    end

    it 'tokenizes strings' do
      Parser.tokens('"foo"').should == [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      Parser.tokens('"this is \"quoted\""').should == [
        { type: :dstr },
        { type: :string, literal: "this is \"quoted\"" },
        { type: :dstrend },
      ]
      Parser.tokens("'foo'").should == [{ type: :string, literal: 'foo' }]
      Parser.tokens("'this is \\'quoted\\''").should == [{ type: :string, literal: "this is 'quoted'" }]
      Parser.tokens('"\t\n"').should == [{ type: :dstr }, { type: :string, literal: "\t\n" }, { type: :dstrend }]
      Parser.tokens("'other escaped chars \\\\ \\n'").should == [
        { type: :string, literal: "other escaped chars \\ \\n" },
      ]
      Parser.tokens('%(foo)').should == [{ type: :string, literal: 'foo' }]
      Parser.tokens('%[foo]').should == [{ type: :string, literal: 'foo' }]
      Parser.tokens('%/foo/').should == [{ type: :string, literal: 'foo' }]
      Parser.tokens('%|foo|').should == [{ type: :string, literal: 'foo' }]
      Parser.tokens('%q(foo)').should == [{ type: :string, literal: 'foo' }]
      Parser.tokens('%Q(foo)').should == [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      Parser.tokens('"#{:foo} bar #{1 + 1}"').should == [
        { type: :dstr },
        { type: :string, literal: '' },
        { type: :evstr },
        { type: :symbol, literal: :foo },
        { type: :"\n" },
        { type: :evstrend },
        { type: :string, literal: ' bar ' },
        { type: :evstr },
        { type: :integer, literal: 1 },
        { type: :'+' },
        { type: :integer, literal: 1 },
        { type: :"\n" },
        { type: :evstrend },
        { type: :dstrend },
      ]
      Parser.tokens(%("foo\#{''}bar")).should == [
        { type: :dstr },
        { type: :string, literal: 'foo' },
        { type: :evstr },
        { type: :string, literal: '' },
        { type: :"\n" },
        { type: :evstrend },
        { type: :string, literal: 'bar' },
        { type: :dstrend },
      ]
      Parser.tokens('"#{1}#{2}"').should == [
        { type: :dstr },
        { type: :string, literal: '' },
        { type: :evstr },
        { type: :integer, literal: 1 },
        { type: :"\n" },
        { type: :evstrend },
        { type: :evstr },
        { type: :integer, literal: 2 },
        { type: :"\n" },
        { type: :evstrend },
        { type: :dstrend },
      ]
    end

    xit 'string interpolation weirdness' do
      Parser.tokens('"#{"foo"}"').should == [
        { type: :dstr },
        { type: :string, literal: '' },
        { type: :evstr },
        { type: :string, literal: 'foo' },
        { type: :"\n" },
        { type: :evstrend },
        { type: :dstrend },
      ]
      Parser.tokens('"#{"}"}"')
      Parser.tokens('"#{ "#{ "#{ 1 + 1 }" }" }"')
    end

    it 'tokenizes backticks and %x()' do
      Parser.tokens('`ls`').should == [{ type: :dxstr }, { type: :string, literal: 'ls' }, { type: :dxstrend }]
      Parser.tokens('%x(ls)').should == [{ type: :dxstr }, { type: :string, literal: 'ls' }, { type: :dxstrend }]
      Parser.tokens("%x(ls \#{path})").should == [
        { type: :dxstr },
        { type: :string, literal: 'ls ' },
        { type: :evstr },
        { type: :name, literal: :path },
        { type: :"\n" },
        { type: :evstrend },
        { type: :dxstrend },
      ]
    end

    it 'tokenizes symbols' do
      [
        :foo,
        :FooBar123,
        :foo_bar,
        :'foo bar',
        :"foo\nbar",
        :foo?,
        :'?foo',
        :foo!,
        :'!foo',
        :'@',
        :@foo,
        :@@foo,
        :'foo@',
        :$foo,
        :'foo$',
        :+,
        :-,
        :*,
        :**,
        :/,
        :==,
        :!=,
        :!,
        :'=',
        :%,
        :$0,
        :[],
        :[]=,
      ].each { |symbol| Parser.tokens(symbol.inspect).should == [{ type: :symbol, literal: symbol }] }
    end

    it 'tokenizes arrays' do
      Parser.tokens("['foo']").should == [{ type: :'[' }, { type: :string, literal: 'foo' }, { type: :']' }]
      Parser.tokens("['foo', 1]").should == [
        { type: :'[' },
        { type: :string, literal: 'foo' },
        { type: :',' },
        { type: :integer, literal: 1 },
        { type: :']' },
      ]
      Parser.tokens("%w[    foo\n 1\t 2  ]").should == [{ type: :'%w', literal: 'foo 1 2' }]
      Parser.tokens("%w|    foo\n 1\t 2  |").should == [{ type: :'%w', literal: 'foo 1 2' }]
      Parser.tokens("%W[    foo\n 1\t 2  ]").should == [{ type: :'%W', literal: 'foo 1 2' }]
      Parser.tokens("%W|    foo\n 1\t 2  |").should == [{ type: :'%W', literal: 'foo 1 2' }]
      Parser.tokens("%i[    foo\n 1\t 2  ]").should == [{ type: :'%i', literal: 'foo 1 2' }]
      Parser.tokens("%I[    foo\n 1\t 2  ]").should == [{ type: :'%I', literal: 'foo 1 2' }]
    end

    it 'tokenizes hashes' do
      Parser.tokens("{ 'foo' => 1, bar: 2 }").should == [
        { type: :'{' },
        { type: :string, literal: 'foo' },
        { type: :'=>' },
        { type: :integer, literal: 1 },
        { type: :',' },
        { type: :symbol_key, literal: :bar },
        { type: :integer, literal: 2 },
        { type: :'}' },
      ]
    end

    it 'tokenizes class variables' do
      Parser.tokens('@@foo').should == [{ type: :cvar, literal: :@@foo }]
    end

    it 'tokenizes instance variables' do
      Parser.tokens('@foo').should == [{ type: :ivar, literal: :@foo }]
    end

    it 'tokenizes global variables' do
      Parser.tokens('$foo').should == [{ type: :gvar, literal: :$foo }]
      Parser.tokens('$0').should == [{ type: :gvar, literal: :$0 }]
      Parser.tokens('$?').should == [{ type: :gvar, literal: :$? }]
    end

    it 'tokenizes dots' do
      Parser.tokens('foo.bar').should == [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :bar },
      ]
      Parser.tokens('foo . bar').should == [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :bar },
      ]
    end

    it 'tokenizes newlines' do
      Parser.tokens("foo\nbar").should == [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :name, literal: :bar },
      ]
      Parser.tokens("foo \n bar").should == [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :name, literal: :bar },
      ]
    end

    it 'ignores newlines that are not significant' do
      Parser.tokens("foo(\n1\n)").should == [
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :integer, literal: 1 },
        { type: :')' },
      ]
    end

    it 'tokenizes semicolons as newlines' do
      Parser.tokens('foo;bar').should == [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :name, literal: :bar },
      ]
      Parser.tokens('foo ; bar').should == [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :name, literal: :bar },
      ]
    end

    it 'does not tokenize comments' do
      tokens = Parser.tokens(<<-END)
        foo # comment 1
        # comment 2
        bar
      END
      tokens.should == [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :"\n" },
        { type: :name, literal: :bar },
        { type: :"\n" },
      ]
      Parser.tokens('# only a comment') # does not get stuck in a loop :^)
    end

    it 'tokenizes lambdas' do
      Parser.tokens('-> { }').should == [{ type: :'->' }, { type: :'{' }, { type: :'}' }]
      Parser.tokens('->(x) { }').should == [
        { type: :'->' },
        { type: :'(' },
        { type: :name, literal: :x },
        { type: :')' },
        { type: :'{' },
        { type: :'}' },
      ]
    end

    it 'tokenizes blocks' do
      Parser.tokens("foo do |x, y|\nx\nend").should == [
        { type: :name, literal: :foo },
        { type: :do },
        { type: :'|' },
        { type: :name, literal: :x },
        { type: :',' },
        { type: :name, literal: :y },
        { type: :'|' },
        { type: :name, literal: :x },
        { type: :"\n" },
        { type: :end },
      ]
      Parser.tokens('foo { |x, y| x }').should == [
        { type: :name, literal: :foo },
        { type: :'{' },
        { type: :'|' },
        { type: :name, literal: :x },
        { type: :',' },
        { type: :name, literal: :y },
        { type: :'|' },
        { type: :name, literal: :x },
        { type: :'}' },
      ]
    end

    it 'tokenizes method names' do
      Parser.tokens('def foo()').should == [
        { type: :def },
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :')' },
      ]
      Parser.tokens('def foo?').should == [{ type: :def }, { type: :name, literal: :foo? }]
      Parser.tokens('def foo!').should == [{ type: :def }, { type: :name, literal: :foo! }]
      Parser.tokens('def foo=').should == [{ type: :def }, { type: :name, literal: :foo }, { type: :'=' }]
      Parser.tokens('def self.foo=').should == [
        { type: :def },
        { type: :self },
        { type: :'.' },
        { type: :name, literal: :foo },
        { type: :'=' },
      ]
      Parser.tokens('def /').should == [{ type: :def }, { type: :/ }]
      Parser.tokens('foo.bar=').should == [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :bar },
        { type: :'=' },
      ]
      Parser.tokens('foo::bar!').should == [
        { type: :name, literal: :foo },
        { type: :'::' },
        { type: :name, literal: :bar! },
      ]
      Parser.tokens('Foo::bar!').should == [
        { type: :constant, literal: :Foo },
        { type: :'::' },
        { type: :name, literal: :bar! },
      ]
      Parser.tokens('bar=1').should == [{ type: :name, literal: :bar }, { type: :'=' }, { type: :integer, literal: 1 }]
      Parser.tokens('nil?').should == [{ type: :name, literal: :nil? }]
      Parser.tokens('foo.nil?').should == [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :nil? },
      ]
    end

    it 'tokenizes heredocs' do
      doc1 = <<END
foo = <<FOO
 1
2
FOO
bar
END
      doc2 = <<END
foo(1, <<-FOO, 2)
 1
2
  FOO
bar
END
      Parser.tokens(doc1).should == [
        { type: :name, literal: :foo },
        { type: :'=' },
        { type: :dstr },
        { type: :string, literal: " 1\n2\n" },
        { type: :dstrend },
        { type: :"\n" },
        { type: :name, literal: :bar },
        { type: :"\n" },
      ]
      Parser.tokens(doc2).should == [
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :integer, literal: 1 },
        { type: :',' },
        { type: :dstr },
        { type: :string, literal: " 1\n2\n" },
        { type: :dstrend },
        { type: :',' },
        { type: :integer, literal: 2 },
        { type: :')' },
        { type: :"\n" },
        { type: :name, literal: :bar },
        { type: :"\n" },
      ]
    end

    it 'stores line and column numbers with each token' do
      Parser.tokens("foo = 1 + 2 # comment\n# comment\nbar.baz", true).should == [
        { type: :name, literal: :foo, line: 0, column: 0 },
        { type: :'=', line: 0, column: 4 },
        { type: :integer, literal: 1, line: 0, column: 6 },
        { type: :'+', line: 0, column: 8 },
        { type: :integer, literal: 2, line: 0, column: 10 },
        { type: :"\n", line: 0, column: 21 },
        { type: :"\n", line: 1, column: 9 },
        { type: :name, literal: :bar, line: 2, column: 0 },
        { type: :'.', line: 2, column: 3 },
        { type: :name, literal: :baz, line: 2, column: 4 },
      ]
    end

    it 'tokenizes examples/fib.rb' do
      fib = File.open('examples/fib.rb').read
      Parser.tokens(fib).should == [
        { type: :def },
        { type: :name, literal: :fib },
        { type: :'(' },
        { type: :name, literal: :n },
        { type: :')' },
        { type: :"\n" },
        { type: :if },
        { type: :name, literal: :n },
        { type: :'==' },
        { type: :integer, literal: 0 },
        { type: :"\n" },
        { type: :integer, literal: 0 },
        { type: :"\n" },
        { type: :elsif },
        { type: :name, literal: :n },
        { type: :'==' },
        { type: :integer, literal: 1 },
        { type: :"\n" },
        { type: :integer, literal: 1 },
        { type: :"\n" },
        { type: :else },
        { type: :"\n" },
        { type: :name, literal: :fib },
        { type: :'(' },
        { type: :name, literal: :n },
        { type: :'-' },
        { type: :integer, literal: 1 },
        { type: :')' },
        { type: :'+' },
        { type: :name, literal: :fib },
        { type: :'(' },
        { type: :name, literal: :n },
        { type: :'-' },
        { type: :integer, literal: 2 },
        { type: :')' },
        { type: :"\n" },
        { type: :end },
        { type: :"\n" },
        { type: :end },
        { type: :"\n" },
        { type: :"\n" },
        { type: :name, literal: :num },
        { type: :'=' },
        { type: :constant, literal: :ARGV },
        { type: :'.' },
        { type: :name, literal: :first },
        { type: :"\n" },
        { type: :name, literal: :puts },
        { type: :name, literal: :fib },
        { type: :'(' },
        { type: :name, literal: :num },
        { type: :'?' },
        { type: :name, literal: :num },
        { type: :'.' },
        { type: :name, literal: :to_i },
        { type: :':' },
        { type: :integer, literal: 25 },
        { type: :')' },
        { type: :"\n" },
      ]
    end
  end
end
