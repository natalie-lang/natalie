require_relative '../spec_helper'

describe 'Object#dup' do
  it 'copies the class' do
    a = []
    a.dup.should be_an_instance_of(Array)
  end

  it 'produces a new object_id' do
    a = []
    a.dup.object_id.should != a.object_id
  end

  it 'copies the data' do
    [1, 2].dup.should == [1, 2]
    { 1 => 2 }.dup.should == { 1 => 2 }
    'foo'.dup.should == 'foo'
  end

  it 'copies instance variables' do
    a = []
    a.instance_variable_set(:@foo, 'foo')
    a.dup.instance_variable_get(:@foo).should == 'foo'
    h = {}
    h.instance_variable_set(:@foo, 'foo')
    h.dup.instance_variable_get(:@foo).should == 'foo'
    s = ''
    s.instance_variable_set(:@foo, 'foo')
    s.dup.instance_variable_get(:@foo).should == 'foo'
  end

  it 'does not copy the singleton class' do
    a = 'foo'
    a.define_singleton_method :bar do
      'bar'
    end
    -> { a.dup.bar }.should raise_error(NoMethodError)
  end

  it 'does not copy the frozen status' do
    a = 'foo'.freeze
    a.dup.frozen?.should be_false
  end
end
