# skip-ruby

require_relative '../spec_helper'

describe 'Parser' do
  describe '#tokens' do
    it 'parses keywords' do
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

    it 'parses division' do
      Parser.tokens('1/2').should == [[:integer, 1], :/, [:integer, 2]]
      Parser.tokens('1 / 2').should == [[:integer, 1], :/, [:integer, 2]]
      Parser.tokens('1 / 2 / 3').should == [[:integer, 1], :/, [:integer, 2], :/, [:integer, 3]]
      Parser.tokens('foo / 2').should == [[:identifier, :foo], :/, [:integer, 2]]
      Parser.tokens('foo /2/').should == [[:identifier, :foo], [:regexp, '2']]
      Parser.tokens('foo/2').should == [[:identifier, :foo], :/, [:integer, 2]]
      Parser.tokens('foo( /2/ )').should == [[:identifier, :foo], :"(", [:regexp, '2'], :")"]
      Parser.tokens('foo 1,/2/').should == [[:identifier, :foo], [:integer, 1], :",", [:regexp, '2']]
    end

    it 'parses operators' do
      Parser.tokens('+ - * / = == ===').should == %i[+ - * / = == ===]
    end

    it 'parses integers' do
      Parser.tokens('1 123').should == [
        [:integer, 1],
        [:integer, 123],
      ]
    end

    it 'parses strings' do
      Parser.tokens('"foo"').should == [[:string, 'foo']]
      Parser.tokens("'foo'").should == [[:string, 'foo']]
    end
  end
end
