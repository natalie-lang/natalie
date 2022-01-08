require_relative '../spec_helper'

def method_that_yields(n)
  yield n
end

def method_containing_a_block_that_yields(n)
  [n].each { |i| yield i }
end

def method_containing_a_deeply_nested_block_that_yields(n)
  [n].each { [n].each { [n].each { |i| yield i } } }
end

describe 'yield' do
  it 'calls the block passed to a method' do
    x = 1
    method_that_yields(2) { |i| x = i }
    x.should == 2
  end

  it 'calls the block passed to a method when yielding from within a block' do
    x = 1
    method_containing_a_block_that_yields(2) { |i| x = i }
    x.should == 2
    method_containing_a_deeply_nested_block_that_yields(3) { |i| x = i }
    x.should == 3
  end
end
