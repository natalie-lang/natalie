require_relative '../spec_helper'

class Foo
  def initialize(x)
    @x = x
    @y = 2
  end

  attr_accessor :x
end

describe 'instance variables' do
  describe '#instance_variables' do
    it 'returns an array of all instance variable names' do
      Foo.new(1).instance_variables.sort.should == %i[@x @y]
    end

    it 'returns an empty array when the object is an integer' do
      1.instance_variables.should == []
    end
  end

  describe '#instance_variable_get' do
    it 'returns the value of an instance variable on an object' do
      foo = Foo.new(10)
      foo.instance_variable_get(:@x).should == 10
    end

    it 'returns nil when the object is an integer' do
      1.instance_variable_get(:@x).should == nil
    end
  end

  describe '#instance_variable_set' do
    it 'sets the value of an instance variable on an object' do
      foo = Foo.new(10)
      foo.instance_variable_set(:@x, 20).should == 20
      foo.x.should == 20
    end

    it 'raises an error when the object is an integer' do
      -> { 1.instance_variable_set(:@x, 20) }.should raise_error(FrozenError, "can't modify frozen Integer: 1")
    end

    it 'raises an error when the object is a frozen' do
      o = Object.new
      o.instance_variable_set(:@x, 10)
      o.freeze
      -> { o.instance_variable_set(:@x, 20) }.should raise_error(
                                                       FrozenError,
                                                       "can't modify frozen Object: #{o.inspect}",
                                                     )
    end
  end
end
