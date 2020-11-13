require_relative '../spec_helper'

describe 'Enumerator' do
  it 'works for infinite sequences' do
    fib = Enumerator.new do |y|
      a = b = 1
      loop do
        y << a
        a, b = b, a + b
      end
    end

    fib.take(5).should == [1, 1, 2, 3, 5]
    fib.take(10).should == [1, 1, 2, 3, 5, 8, 13, 21, 34, 55]
  end

  it 'works for finite sequences' do
    numbers = Enumerator.new do |y|
      y << 1
      y << 2
      y << 3
    end

    numbers.to_a.should == [1, 2, 3]
  end
end
