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
end
