require_relative '../spec_helper'

describe 'symbol' do
  it 'can be built dynamically' do
    :"foo #{1 + 1}".should == :"foo 2"
  end

  describe '#inspect' do
    it 'returns a code representation of the symbol' do
      :foo.inspect.should == ':foo'
      :foo_bar.inspect.should == ':foo_bar'
      :"foo bar".inspect.should == ':"foo bar"'
      :"foo\nbar".inspect.should == ":\"foo\\nbar\""
    end
  end

  describe '#to_s' do
    it 'returns the symbol as a string' do
      :foo.to_s.should == 'foo'
      :"foo bar".to_s.should == 'foo bar'
      :"foo\nbar".to_s.should == "foo\nbar"
    end
  end

  describe '#to_proc' do
    it 'returns a proc that calls a method on its given argument' do
      :to_i.to_proc.call('2').should == 2
    end
  end
end
