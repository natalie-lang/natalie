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
  end
end
