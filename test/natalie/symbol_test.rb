require_relative '../spec_helper'

describe 'symbol' do
  it 'can be built dynamically' do
    :"foo #{1 + 1}".should == :'foo 2'
  end

  describe '#inspect' do
    it 'returns a code representation of the symbol' do
      :foo.inspect.should == ':foo'
      :FooBar123.inspect.should == ':FooBar123'
      :foo_bar.inspect.should == ':foo_bar'
      :'foo bar'.inspect.should == ':"foo bar"'
      :"foo\nbar".inspect.should == ":\"foo\\nbar\""
      :foo=.inspect.should == ':foo='
      :foo?.inspect.should == ':foo?'
      :'?foo'.inspect.should == ':"?foo"'
      :foo!.inspect.should == ':foo!'
      :'!foo'.inspect.should == ':"!foo"'
      :'@'.inspect.should == ':"@"'
      :@foo.inspect.should == ':@foo'
      :@@foo.inspect.should == ':@@foo'
      :'foo@'.inspect.should == ':"foo@"'
      :$foo.inspect.should == ':$foo'
      :'foo$'.inspect.should == ':"foo$"'
      :+.inspect.should == ':+'
      :-.inspect.should == ':-'
      :*.inspect.should == ':*'
      :**.inspect.should == ':**'
      :/.inspect.should == ':/'
      :==.inspect.should == ':=='
      :!=.inspect.should == ':!='
      :!.inspect.should == ':!'
      :'='.inspect.should == ':"="'
      :%.inspect.should == ':%'
      :$0.inspect.should == ':$0'
      :[].inspect.should == ':[]'
      :[]=.inspect.should == ':[]='
      :<<.inspect.should == ':<<'
      :<.inspect.should == ':<'
      :>>.inspect.should == ':>>'
      :>.inspect.should == ':>'
      :$?.inspect.should == ':$?'
      :$!.inspect.should == ':$!'
      :$~.inspect.should == ':$~'
      (:&).inspect.should == ':&'
    end
  end

  describe '#to_s' do
    it 'returns the symbol as a string' do
      :foo.to_s.should == 'foo'
      :'foo bar'.to_s.should == 'foo bar'
      :"foo\nbar".to_s.should == "foo\nbar"
    end
  end

  describe '#to_proc' do
    it 'returns a proc that calls a method on its given argument' do
      :to_i.to_proc.call('2').should == 2
    end
  end

  describe '#start_with?' do
    it 'returns true if the symbol (when converted to a string) starts with the given substring' do
      :tim.start_with?('t').should be_true
      :tim.start_with?('tim').should be_true
      :tim.start_with?('').should be_true
      :tim.start_with?('xxxxx').should be_false
    end
  end

  describe '#[]' do
    it 'subscripts the symbol as if it were a string' do
      :foo_bar[0..2].should == 'foo'
      :foo_bar[1..-2].should == 'oo_ba'
      :foo_bar[0].should == 'f'
      :foo_bar[-1].should == 'r'
      :n[1..-1].should == ''
    end
  end
end
