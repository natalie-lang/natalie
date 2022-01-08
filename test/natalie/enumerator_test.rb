require_relative '../spec_helper'

class Thing
  include Enumerable

  def each
    yield 1
    yield 2
    yield 3
  end
end

class YieldMultiple
  include Enumerable

  def each
    yield 1, 2
    yield 3, 4
  end
end

describe 'Enumerator' do
  it 'works for infinite sequences' do
    fib =
      Enumerator.new do |y|
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
    numbers =
      Enumerator.new do |y|
        y << 1
        y << 2
        y << 3
      end

    numbers.to_a.should == [1, 2, 3]
  end

  describe '#next' do
    it 'returns the next item yielded by the enumerator and moves the internal position forward' do
      numbers =
        Enumerator.new do |y|
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

    it 'works for map' do
      e = Thing.new.map
      e.next.should == 1
      e.next.should == 2
      e.next.should == 3

      e.rewind
      r = e.each { |i| i * 2 }
      r.should == [2, 4, 6]
    end
  end

  describe '#peek' do
    it 'returns the next item yielded by the enumerator but does not move the internal position forward' do
      numbers =
        Enumerator.new do |y|
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
      numbers =
        Enumerator.new do |y|
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

  it 'returns the final result' do
    t = Thing.new
    fail_proc = -> { 'result' }
    t.find(fail_proc) { |e| false }.should == 'result'
    t.find(fail_proc).each { |e| false }.should == 'result'
  end

  it 'should break' do
    x = 0
    e = loop
    e.each do
      x += 1
      break if x > 3
    end
    x.should == 4
  end

  it 'can be chained' do
    e = %i[a b].each_with_index
    e.to_a.should == [[:a, 0], [:b, 1]]

    r = %i[a b].each_with_index.map { |x, i| [x, i] }
    r.should == [[:a, 0], [:b, 1]]
  end

  it 'can yield multiple' do
    YieldMultiple.new.max.should == [3, 4]
  end

  describe 'Lazy' do
    it 'rewind enum before looping' do
      enum = (1..5).lazy
      select = enum.select { true }
      select.force.should == [1, 2, 3, 4, 5]
      enum.force.should == [1, 2, 3, 4, 5]
    end

    describe '#zip' do
      it 'not raise TypeError if argument converted by to_ary responds to :each' do
        class ArrayConvertible
          def to_ary
            []
          end
        end
        -> { [].lazy.zip(ArrayConvertible.new) }.should_not raise_error(TypeError)
      end
    end
  end
end
