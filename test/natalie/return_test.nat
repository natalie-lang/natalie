require_relative '../spec_helper'

describe 'return' do
  it 'returns early from a method' do
    def foo
      return 'foo'
      'bar'
    end
    foo.should == 'foo'
  end

  it 'returns early from a block' do
    def one
      [1, 2, 3].each do |i|
        [1, 2, 3].each do |i|
          return i if i == 1
        end
      end
    end
    one.should == 1
  end

  it 'handles other errors properly' do
    def foo(x)
      [1].each do |i|
        return i if x == i
      end
      raise 'foo'
    end
    -> { foo(2) }.should raise_error(StandardError)
    foo(1).should == 1
  end
end
