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

  describe '#next' do
    it 'returns the next item yielded by the enumerator and moves the internal position forward' do
      numbers = Enumerator.new do |y|
        y << 1
        y << 2
        y << 3
        'this is ignored'
      end
      numbers.next.should == 1
      numbers.next.should == 2
      numbers.next.should == 3
      -> { numbers.next }.should raise_error(StopIteration)
    end
  end

  describe '#peek' do
    it 'returns the next item yielded by the enumerator but does not move the internal position forward' do
      numbers = Enumerator.new do |y|
        y << 1
        y << 2
        y << 3
        'this is ignored'
      end
      numbers.peek.should == 1
      numbers.next
      numbers.peek.should == 2
      numbers.next
      numbers.peek.should == 3
      numbers.next
      -> { numbers.peek }.should raise_error(StopIteration)
    end
  end

  describe '#rewind' do
    it 'rewinds the sequence to the beginning' do
      numbers = Enumerator.new do |y|
        y << 1
        y << 2
        y << 3
        'this is ignored'
      end
      numbers.next
      numbers.next
      numbers.rewind
      numbers.next.should == 1
      numbers.next.should == 2
    end
  end
end
