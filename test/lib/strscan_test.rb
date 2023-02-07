require_relative '../spec_helper'
require 'strscan'

describe 'StringScanner' do
  it 'works' do
    s = StringScanner.new('This is an example string')
    s.eos?.should == false
    s.scan(/\w+/).should == 'This'
    s.scan(/\w+/).should == nil
    s.scan(/\s+/).should == ' '
    s.scan(/\s+/).should == nil
    s.scan(/\w+/).should == 'is'
    s.eos?.should == false
    s.scan(/\s+/).should == ' '
    s.scan(/\w+/).should == 'an'
    s.scan(/\s+/).should == ' '
    s.scan(/\w+/).should == 'example'
    s.scan(/\s+/).should == ' '
    s.scan(/\w+/).should == 'string'
    s.eos?.should == true
    s.scan(/\s+/).should == nil
    s.scan(/\w+/).should == nil
  end

  describe '#captures' do
    it 'works' do
      s = StringScanner.new('Fri Dec 12 1975 14:39')
      s.scan(/(\w+) (\w+) (\d+) /)
      s.captures.should == ['Fri', 'Dec', '12']
      s.scan(/(\w+) (\w+) (\d+) /)
      s.captures.should == nil
    end
  end

  describe '#scan_until' do
    it 'works with zero length matches' do
      s = StringScanner.new('foo bar')
      s.scan_until(/(?=\s)/).should == 'foo'
    end
  end
end
