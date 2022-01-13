require_relative '../spec_helper'

describe 'define_singleton_method' do
  it 'defines a new method on the object given a name and block' do
    obj = Object.new
    obj.define_singleton_method(:foo) { 'foo method' }
    obj.foo.should == 'foo method'
  end
end
