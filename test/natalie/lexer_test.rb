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
        Parser.tokens(keyword).should == [{type: keyword.to_sym}]
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
      Parser.tokens('+ - * / = == ===').should == [
        {type: :"+"},
        {type: :"-"},
        {type: :"*"},
        {type: :"/"},
        {type: :"="},
        {type: :"=="},
        {type: :"==="},
      ]
    end

    it 'tokenizes integers' do
      Parser.tokens('1 123 -456').should == [
        {type: :integer, literal: 1},
        {type: :integer, literal: 123},
        {type: :"-"}, # NOTE: we cannot tokenize a negative number without knowing more context
        {type: :integer, literal: 456}
      ]
    end

    it 'tokenizes strings' do
      Parser.tokens('"foo"').should == [{type: :string, literal: 'foo'}]
      Parser.tokens("'foo'").should == [{type: :string, literal: 'foo'}]
    end

    it 'tokenizes arrays' do
      Parser.tokens('["foo"]').should == [{type: :"["}, {type: :string, literal: 'foo'}, {type: :"]"}]
      Parser.tokens('["foo", 1]').should == [{type: :"["}, {type: :string, literal: 'foo'}, {type: :","}, {type: :integer, literal: 1}, {type: :"]"}]
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
  end
end
