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
        Parser.tokens(keyword).should == [keyword.to_sym]
      end
      Parser.tokens('defx = 1').should == [[:identifier, :defx], :"=", [:integer, 1]]
    end

    it 'tokenizes division' do
      Parser.tokens('1/2').should == [[:integer, 1], :/, [:integer, 2]]
      Parser.tokens('1 / 2').should == [[:integer, 1], :/, [:integer, 2]]
      Parser.tokens('1 / 2 / 3').should == [[:integer, 1], :/, [:integer, 2], :/, [:integer, 3]]
      Parser.tokens('foo / 2').should == [[:identifier, :foo], :/, [:integer, 2]]
      Parser.tokens('foo /2/').should == [[:identifier, :foo], [:regexp, '2']]
      Parser.tokens('foo/2').should == [[:identifier, :foo], :/, [:integer, 2]]
      Parser.tokens('foo( /2/ )').should == [[:identifier, :foo], :"(", [:regexp, '2'], :")"]
      Parser.tokens('foo 1,/2/').should == [[:identifier, :foo], [:integer, 1], :",", [:regexp, '2']]
    end

    it 'tokenizes operators' do
      Parser.tokens('+ - * / = == ===').should == %i[+ - * / = == ===]
    end

    it 'tokenizes integers' do
      Parser.tokens('1 123 -456').should == [
        [:integer, 1],
        [:integer, 123],
        :"-", # NOTE: we cannot tokenize a negative number without knowing more context
        [:integer, 456]
      ]
    end

    it 'tokenizes strings' do
      Parser.tokens('"foo"').should == [[:string, 'foo']]
      Parser.tokens("'foo'").should == [[:string, 'foo']]
    end

    it 'tokenizes arrays' do
      Parser.tokens('["foo"]').should == [:"[", [:string, 'foo'], :"]"]
      Parser.tokens('["foo", 1]').should == [:"[", [:string, 'foo'], :",", [:integer, 1], :"]"]
    end

    it 'tokenizes hashes' do
      Parser.tokens('{ "foo" => 1, bar: 2 }').should == [
        :"{",
        [:string, 'foo'], :"=>", [:integer, 1],
        :",",
        [:symbol_key, :bar], [:integer, 2], :"}"
      ]
    end

    it 'tokenizes dots' do
      Parser.tokens('foo.bar').should == [[:identifier, :foo], :".", [:identifier, :bar]]
      Parser.tokens('foo . bar').should == [[:identifier, :foo], :".", [:identifier, :bar]]
    end

    it 'tokenizes newlines' do
      Parser.tokens("foo\nbar").should == [[:identifier, :foo], :"\n", [:identifier, :bar]]
      Parser.tokens("foo \n bar").should == [[:identifier, :foo], :"\n", [:identifier, :bar]]
    end

    it 'tokenizes semicolons' do
      Parser.tokens("foo;bar").should == [[:identifier, :foo], :";", [:identifier, :bar]]
      Parser.tokens("foo ; bar").should == [[:identifier, :foo], :";", [:identifier, :bar]]
    end

    it 'does not tokenize comments' do
      tokens = Parser.tokens(<<-END)
        foo # comment 1
        # comment 2
        bar
      END
      tokens.should == [[:identifier, :foo], :"\n", :"\n", [:identifier, :bar], :"\n"]
    end
  end
end
