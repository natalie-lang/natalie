require_relative '../spec_helper'

describe 'MatchData' do
  describe '#captures' do
    /foo/.match('foo').captures.should == []
    /f(.)(o)/.match('foo').captures.should == ['o', 'o']
  end

  describe '#size' do
    it 'returns the number of captures (including the whole match)' do
      match = /foo/.match('foo')
      match.size.should == 1
      match = /bar/.match('foo bar baz')
      match.size.should == 1
      match = /bar (baz)/.match('foo bar baz')
      match.size.should == 2
    end
  end

  describe '#length' do
    it 'returns the number of captures (including the whole match)' do
      match = /foo/.match('foo')
      match.length.should == 1
      match = /bar/.match('foo bar baz')
      match.length.should == 1
      match = /bar (baz)/.match('foo bar baz')
      match.length.should == 2
    end
  end

  describe '#to_s' do
    it 'returns the matched string' do
      match = /foo/.match('foo')
      match.to_s.should == 'foo'
      match = /bar/.match('foo bar baz')
      match.to_s.should == 'bar'
      match = /bar (baz)/.match('foo bar baz')
      match.to_s.should == 'bar baz'
    end
  end

  describe '#[]' do
    it 'returns a capture group' do
      match = /(foo) bar (baz)/.match('foo bar baz')
      match[0].should == 'foo bar baz'
      match[1].should == 'foo'
      match[2].should == 'baz'
      match[3].should == nil
    end

    it 'supports exclusive ranges' do
      match = /(foo) bar (baz)/.match('foo bar baz')
      match[0...2].should == ['foo bar baz', 'foo']
    end

    it 'supports ranges with negative end values' do
      match = /(foo) bar (baz)/.match('foo bar baz')
      match[1..-1].should == ['foo', 'baz']
      match[1..-2].should == ['foo']
      match[1..-3].should == []
      match[1..-4].should == []

      match[1...-1].should == ['foo']
      match[1...-2].should == []
      match[1...-3].should == []
    end

    it 'supports ranges that results in added nil values' do
      match = /(foo) (bar )?(baz)/.match('foo baz')
      match[0..].should == ['foo baz', 'foo', nil, 'baz']
      match[0..3].should == ['foo baz', 'foo', nil, 'baz']
      match[0...4].should == ['foo baz', 'foo', nil, 'baz']
      match[0..-1].should == ['foo baz', 'foo', nil, 'baz']

      match = /(foo) bar(baz )?/.match('foo bar')
      match[0..].should == ['foo bar', 'foo', nil]
      match[0..2].should == ['foo bar', 'foo', nil]
      match[0...3].should == ['foo bar', 'foo', nil]
      match[0..-1].should == ['foo bar', 'foo', nil]
    end

    it 'support ranges when using optional capture groups' do
      md = /(foo)?(bar)/.match('bar')
      md[1..-1].should == [nil, 'bar']
    end
  end
end
