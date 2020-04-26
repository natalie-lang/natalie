require_relative '../spec_helper'

NUM = 1

class Foo
end

describe 'const_defined?' do
  it 'works' do
    Object.const_defined?('NUM').should == true
    Object.const_defined?('Foo').should == true
    Object.const_defined?('Bar').should == false
  end
end
