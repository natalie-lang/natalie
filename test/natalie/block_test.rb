require_relative '../spec_helper'

class ArrayLike
  def initialize(*values)
    @values = values
  end

  def to_ary
    @values
  end
end

describe 'block' do
  describe 'arguments' do
    it 'can assign any number of arguments' do
      proc { |a| a }.call.should == nil
      proc { |a| a }.call(1).should == 1
      proc { |a| a }.call(1, 2).should == 1
      proc { |a, b| [a, b] }.call.should == [nil, nil]
      proc { |a, b| [a, b] }.call(1).should == [1, nil]
      proc { |a, b| [a, b] }.call(1, 2).should == [1, 2]
      proc { |a, b| [a, b] }.call(1, 2, 3).should == [1, 2]
      proc { |a,| a }.call(1, 2).should == 1
      proc { |a,| a }.call([3, 4]).should == 3
    end

    it 'can assign with argument defaults on the left' do
      proc { |a = 1| a }.call.should == 1
      proc { |a = 14, b| [a, b] }.call.should == [14, nil]
      proc { |a = 15, b| [a, b] }.call(100).should == [15, 100]
      proc { |a = 16, b| [a, b] }.call(1, 2).should == [1, 2]
      proc { |a = 17, b| [a, b] }.call(1, 2, 3).should == [1, 2]
      proc { |a = 18, b| [a, b] }.call([1]).should == [18, 1]
      proc { |a = 19, b| [a, b] }.call([1, 2]).should == [1, 2]
      proc { |a = 20, b| [a, b] }.call([1, 2, 3]).should == [1, 2]
      proc { |a = 21, b = 22, c, d| [a, b, c, d] }.call([3]).should == [21, 22, 3, nil]
      proc { |a = 23, b = 24, c, d| [a, b, c, d] }.call([3, 4]).should == [23, 24, 3, 4]
      proc { |a = 25, b = 26, c, d| [a, b, c, d] }.call([3, 4, 5]).should == [3, 26, 4, 5]
      proc { |a = 8, b = 9, c, d, e, f| [a, b, c, d, e, f] }.call(1, 2, 3, 4, 5, 6).should == [1, 2, 3, 4, 5, 6]
      proc { |a = 10, b = 20, c, d, e, f| [a, b, c, d, e, f] }.call(2, 3, 4, 5, 6).should == [2, 20, 3, 4, 5, 6]
      proc { |a = 30, b = 40, c, d, e, f| [a, b, c, d, e, f] }.call(3, 4, 5, 6).should == [30, 40, 3, 4, 5, 6]
      proc { |a = 50, b = 60, c, d, e, f| [a, b, c, d, e, f] }.call(3, 4, 5).should == [50, 60, 3, 4, 5, nil]
      proc { |a = 70, b = 80, c, d, e, f| [a, b, c, d, e, f] }.call(3, 4).should == [70, 80, 3, 4, nil, nil]
      proc { |a = 90, b = 100, c, d, e, f| [a, b, c, d, e, f] }.call(3).should == [90, 100, 3, nil, nil, nil]
      proc { |a = 110, b = 120, c, d, e, f| [a, b, c, d, e, f] }.call.should == [110, 120, nil, nil, nil, nil]
    end

    it 'can assign with argument defaults on the right' do
      proc { |a, b = 1| [a, b] }.call.should == [nil, 1]
      proc { |a, b = 12| [a, b] }.call(1).should == [1, 12]
      proc { |a, b = 13| [a, b] }.call(1, 2, 3).should == [1, 2]
      proc { |a, b, c, d, e = 14, f = 15| [a, b, c, d, e, f] }.call(1, 2, 3, 4, 5, 6).should == [1, 2, 3, 4, 5, 6]
      proc { |a, b, c, d, e = 16, f = 17| [a, b, c, d, e, f] }.call(1, 2, 3, 4, 5).should == [1, 2, 3, 4, 5, 17]
      proc { |a, b, c, d, e = 18, f = 19| [a, b, c, d, e, f] }.call(1, 2, 3, 4).should == [1, 2, 3, 4, 18, 19]
      proc { |a, b, c, d, e = 20, f = 21| [a, b, c, d, e, f] }.call(1, 2, 3).should == [1, 2, 3, nil, 20, 21]
      proc { |a, b, c, d, e = 22, f = 23| [a, b, c, d, e, f] }.call(1, 2).should == [1, 2, nil, nil, 22, 23]
      proc { |a, b, c, d, e = 24, f = 25| [a, b, c, d, e, f] }.call(1).should == [1, nil, nil, nil, 24, 25]
      proc { |a, b, c, d, e = 26, f = 27| [a, b, c, d, e, f] }.call.should == [nil, nil, nil, nil, 26, 27]
    end

    it 'can match arrays inside arguments' do
      proc { |a, (b, c)| [a, b, c] }.call.should == [nil, nil, nil]
      proc { |a, (b, c)| [a, b, c] }.call(1).should == [1, nil, nil]
      proc { |a, (b, c)| [a, b, c] }.call(1, [2]).should == [1, 2, nil]
      proc { |a, (b, c)| [a, b, c] }.call(1, [2, 3]).should == [1, 2, 3]
      proc { |a, (b, c)| [a, b, c] }.call([1]).should == [1, nil, nil]
      proc { |a, (b, c)| [a, b, c] }.call([1, [2]]).should == [1, 2, nil]
      proc { |a, (b, c)| [a, b, c] }.call([1, [2, 3]]).should == [1, 2, 3]
    end
  end

  describe 'passing a symbol as a block' do
    ['2'].map(&:to_i).should == [2]
  end

  describe 'method call with block || another value' do
    it 'works' do
      r = [1, 2].any? { |i| i > 2 } || 'nope'
      r.should == 'nope'
    end
  end

  describe 'arity' do
    def arity(&block)
      block.arity
    end

    it 'works' do
      proc {}.arity.should == 0
      proc { || }.arity.should == 0
      proc { |a| }.arity.should == 1
      proc { |a, b| }.arity.should == 2
      proc { |a, b, c| }.arity.should == 3
      proc { |*a| }.arity.should == -1
      proc { |a, *b| }.arity.should == -2
      proc { |a, *b, c| }.arity.should == -3
      proc { |x:, y:, z: 0| }.arity.should == 1
      proc { |*a, x:, y: 0| }.arity.should == -2
      proc { |a = 0| }.arity.should == 0
      lambda { |a = 0| }.arity.should == -1
      proc { |a = 0, b| }.arity.should == 1
      lambda { |a = 0, b| }.arity.should == -2
      proc { |a = 0, b = 0| }.arity.should == 0
      lambda { |a = 0, b = 0| }.arity.should == -1
      proc { |a, b = 0| }.arity.should == 1
      lambda { |a, b = 0| }.arity.should == -2
      proc { |(a, b), c = 0| }.arity.should == 1
      lambda { |(a, b), c = 0| }.arity.should == -2
      proc { |a, x: 0, y: 0| }.arity.should == 1
      lambda { |a, x: 0, y: 0| }.arity.should == -2
    end
  end
end
