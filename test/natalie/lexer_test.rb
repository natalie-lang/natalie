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
      ].each do |keyword|
        Parser.tokens(keyword).should == [{type: :keyword, literal: keyword.to_sym}]
      end
      Parser.tokens('defx = 1').should == [{type: :identifier, literal: :defx}, {type: :"="}, {type: :integer, literal: 1}]
    end

    it 'tokenizes division and regexp' do
      Parser.tokens('1/2').should == [{type: :integer, literal: 1}, {type: :"/"}, {type: :integer, literal: 2}]
      Parser.tokens('1 / 2').should == [{type: :integer, literal: 1}, {type: :"/"}, {type: :integer, literal: 2}]
      Parser.tokens('1 / 2 / 3').should == [{type: :integer, literal: 1}, {type: :"/"}, {type: :integer, literal: 2}, {type: :"/"}, {type: :integer, literal: 3}]
      Parser.tokens('foo / 2').should == [{type: :identifier, literal: :foo}, {type: :"/"}, {type: :integer, literal: 2}]
      Parser.tokens('foo /2/').should == [{type: :identifier, literal: :foo}, {type: :regexp, literal: '2'}]
      Parser.tokens('foo/2').should == [{type: :identifier, literal: :foo}, {type: :"/"}, {type: :integer, literal: 2}]
      Parser.tokens('foo( /2/ )').should == [{type: :identifier, literal: :foo}, {type: :"("}, {type: :regexp, literal: '2'}, {type: :")"}]
      Parser.tokens('foo 1,/2/').should == [{type: :identifier, literal: :foo}, {type: :integer, literal: 1}, {type: :","}, {type: :regexp, literal: '2'}]
    end

    it 'tokenizes operators' do
      operators = %w[+ += - -= * *= ** **= / /= % %= = == === != > >= < <= <=> & | ^ ~ << >> && || ! ? : ::]
      Parser.tokens(operators.join(' ')).should == operators.map { |o| {type: o.to_sym} }
    end

    it 'tokenizes integers' do
      Parser.tokens('1 123 -456 - 0').should == [
        {type: :integer, literal: 1},
        {type: :integer, literal: 123},
        # NOTE: in the parser this negative number may need to be interpreted to a minus (-) followed by a literal 456,
        # depending on if the previous token is a variable or a method name
        {type: :integer, literal: -456},
        {type: :"-"},
        {type: :integer, literal: 0},
      ]
    end

    it 'tokenizes strings' do
      Parser.tokens('"foo"').should == [{type: :string, literal: 'foo'}]
      Parser.tokens("'foo'").should == [{type: :string, literal: 'foo'}]
      Parser.tokens("%(foo)").should == [{type: :string, literal: 'foo'}]
      Parser.tokens("%[foo]").should == [{type: :string, literal: 'foo'}]
      Parser.tokens("%/foo/").should == [{type: :string, literal: 'foo'}]
      Parser.tokens("%q(foo)").should == [{type: :string, literal: 'foo'}]
      Parser.tokens("%Q(foo)").should == [{type: :string, literal: 'foo'}]
    end

    it 'tokenizes arrays' do
      Parser.tokens('["foo"]').should == [{type: :"["}, {type: :string, literal: 'foo'}, {type: :"]"}]
      Parser.tokens('["foo", 1]').should == [{type: :"["}, {type: :string, literal: 'foo'}, {type: :","}, {type: :integer, literal: 1}, {type: :"]"}]
      Parser.tokens('%w[foo 1]').should == [{type: :"%w", literal: 'foo 1'}]
      Parser.tokens('%W[foo 1]').should == [{type: :"%W", literal: 'foo 1'}]
      Parser.tokens('%i[foo 1]').should == [{type: :"%i", literal: 'foo 1'}]
    end

    it 'tokenizes hashes' do
      Parser.tokens('{ "foo" => 1, bar: 2 }').should == [
        {type: :"{"},
        {type: :string, literal: 'foo'}, {type: :"=>"}, {type: :integer, literal: 1},
        {type: :","},
        {type: :symbol_key, literal: :bar}, {type: :integer, literal: 2},
        {type: :"}"}
      ]
    end

    it 'tokenizes class variables' do
      Parser.tokens('@@foo').should == [{type: :cvar, literal: :@@foo}]
    end

    it 'tokenizes instance variables' do
      Parser.tokens('@foo').should == [{type: :ivar, literal: :@foo}]
    end

    it 'tokenizes global variables' do
      Parser.tokens('$foo').should == [{type: :gvar, literal: :$foo}]
    end

    it 'tokenizes dots' do
      Parser.tokens('foo.bar').should == [{type: :identifier, literal: :foo}, {type: :"."}, {type: :identifier, literal: :bar}]
      Parser.tokens('foo . bar').should == [{type: :identifier, literal: :foo}, {type: :"."}, {type: :identifier, literal: :bar}]
    end

    it 'tokenizes newlines' do
      Parser.tokens("foo\nbar").should == [{type: :identifier, literal: :foo}, {type: :"\n"}, {type: :identifier, literal: :bar}]
      Parser.tokens("foo \n bar").should == [{type: :identifier, literal: :foo}, {type: :"\n"}, {type: :identifier, literal: :bar}]
    end

    it 'tokenizes semicolons' do
      Parser.tokens("foo;bar").should == [{type: :identifier, literal: :foo}, {type: :";"}, {type: :identifier, literal: :bar}]
      Parser.tokens("foo ; bar").should == [{type: :identifier, literal: :foo}, {type: :";"}, {type: :identifier, literal: :bar}]
    end

    it 'does not tokenize comments' do
      tokens = Parser.tokens(<<-END)
        foo # comment 1
        # comment 2
        bar
      END
      tokens.should == [{type: :identifier, literal: :foo}, {type: :"\n"}, {type: :"\n"}, {type: :identifier, literal: :bar}, {type: :"\n"}]
      Parser.tokens("# only a comment") # does not get stuck in a loop :^)
    end

    it 'tokenizes lambdas' do
      Parser.tokens('-> { }').should == [{type: :"->"}, {type: :"{"}, {type: :"}"}]
      Parser.tokens('->(x) { }').should == [{type: :"->"}, {type: :"("}, {type: :identifier, literal: :x}, {type: :")"}, {type: :"{"}, {type: :"}"}]
    end

    it 'tokenizes blocks' do
      Parser.tokens("foo do |x, y|\nx\nend").should == [
        {type: :identifier, literal: :foo},
        {type: :keyword, literal: :do},
        {type: :"|"},
        {type: :identifier, literal: :x},
        {type: :","},
        {type: :identifier, literal: :y},
        {type: :"|"},
        {type: :"\n"},
        {type: :identifier, literal: :x},
        {type: :"\n"},
        {type: :keyword, literal: :end},
      ]
      Parser.tokens("foo { |x, y| x }").should == [
        {type: :identifier, literal: :foo},
        {type: :"{"},
        {type: :"|"},
        {type: :identifier, literal: :x},
        {type: :","},
        {type: :identifier, literal: :y},
        {type: :"|"},
        {type: :identifier, literal: :x},
        {type: :"}"},
      ]
    end

    it 'stores line and column numbers with each token' do
      Parser.tokens("foo = 1 + 2 # comment\n# comment\nbar.baz", true).should == [
        {type: :identifier, literal: :foo, line: 0, column: 0},
        {type: :"=", line: 0, column: 4},
        {type: :integer, literal: 1, line: 0, column: 6},
        {type: :"+", line: 0, column: 8},
        {type: :integer, literal: 2, line: 0, column: 10},
        {type: :"\n", line: 0, column: 21},
        {type: :"\n", line: 1, column: 9},
        {type: :identifier, literal: :bar, line: 2, column: 0},
        {type: :".", line: 2, column: 3},
        {type: :identifier, literal: :baz, line: 2, column: 4},
      ]
    end

    it 'tokenizes examples/fib.rb' do
      fib = File.open('examples/fib.rb').read
      Parser.tokens(fib).should == [
        {type: :keyword, literal: :def},
        {type: :identifier, literal: :fib},
        {type: :"("},
        {type: :identifier, literal: :n},
        {type: :")"},
        {type: :"\n"},
        {type: :keyword, literal: :if},
        {type: :identifier, literal: :n},
        {type: :"=="},
        {type: :integer, literal: 0},
        {type: :"\n"},
        {type: :integer, literal: 0},
        {type: :"\n"},
        {type: :keyword, literal: :elsif},
        {type: :identifier, literal: :n},
        {type: :"=="},
        {type: :integer, literal: 1},
        {type: :"\n"},
        {type: :integer, literal: 1},
        {type: :"\n"},
        {type: :keyword, literal: :else},
        {type: :"\n"},
        {type: :identifier, literal: :fib},
        {type: :"("},
        {type: :identifier, literal: :n},
        {type: :"-"},
        {type: :integer, literal: 1},
        {type: :")"},
        {type: :"+"},
        {type: :identifier, literal: :fib},
        {type: :"("},
        {type: :identifier, literal: :n},
        {type: :"-"},
        {type: :integer, literal: 2},
        {type: :")"},
        {type: :"\n"},
        {type: :keyword, literal: :end},
        {type: :"\n"},
        {type: :keyword, literal: :end},
        {type: :"\n"},
        {type: :"\n"},
        {type: :identifier, literal: :num},
        {type: :"="},
        {type: :constant, literal: :ARGV},
        {type: :"."},
        {type: :identifier, literal: :first},
        {type: :"\n"},
        {type: :identifier, literal: :puts},
        {type: :identifier, literal: :fib},
        {type: :"("},
        {type: :identifier, literal: :num},
        {type: :"?"},
        {type: :identifier, literal: :num},
        {type: :"."},
        {type: :identifier, literal: :to_i},
        {type: :":"},
        {type: :integer, literal: 25},
        {type: :")"},
        {type: :"\n"}
      ]
    end
  end
end
