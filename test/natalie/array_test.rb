require_relative '../spec_helper'

describe 'array' do
  it 'can be assigned from a splat (makes a copy)' do
    a = *[1, 2, 3]
    b = *a
    b.should == a
    b.object_id.should != a.object_id
  end

  it 'can be assigned from a splat of a single item' do
    a = 1
    b = *a
    b.should == [1]
  end

  describe 'Array()' do
    it 'returns an empty array given nil' do
      Array(nil).should == []
    end

    it 'returns the array when given an array' do
      ary = []
      ary.should_not_receive(:to_ary)
      Array(ary).should == []
      Array([1, 2, 3]).should == [1, 2, 3]
    end

    it 'returns an array wrapping other types of items' do
      Array(1).should == [1]
      Array(:foo).should == [:foo]
      Array(/regex/).should == [/regex/]
      o = Object.new
      Array(o).should == [o]
    end

    it 'returns the implicitly converted array if the object responds to to_ary' do
      n = Object.new
      n.should_receive(:to_ary).and_return([3, 4])
      Array(n).should == [3, 4]
      o = mock('thing that responds to to_ary')
      o.should_receive(:to_ary).and_return([10, 20])
      Array(o).should == [10, 20]
    end
  end

  describe '<=>' do
    it 'returns -1, 0, or 1 depending on array elements' do
      ([] <=> []).should == 0
      ([1] <=> []).should == 1
      ([] <=> [1]).should == -1
      ([1] <=> [1]).should == 0
      ([1] <=> [2]).should == -1
      ([2] <=> [1]).should == 1
      ([1, 2] <=> [1]).should == 1
      ([1] <=> [1, 2]).should == -1
      (%w[a b c] <=> %w[a b c]).should == 0
      (%w[a b] <=> %w[a b c]).should == -1
      (%w[a b c] <=> %w[a b]).should == 1
      (%w[a b C] <=> %w[a b c]).should == -1
    end

    it 'raises TypeError if #to_ary returns a non-array' do
      x = Object.new
      def x.to_ary
        :sym
      end

      -> { [] <=> x }.should raise_error(TypeError, "can't convert Object to Array (Object#to_ary gives Symbol)")
    end
  end

  describe '#to_a' do
    it 'returns self' do
      a = [1, 2, 3]
      a2 = a.to_a
      a2.object_id.should == a.object_id
      a2.should == a
    end
  end

  describe '#flatten' do
    it 'raises TypeError if #to_ary of an element returns a non-array' do
      x = Object.new
      def x.to_ary
        :sym
      end

      -> { [x].flatten }.should raise_error(TypeError, "can't convert Object to Array (Object#to_ary gives Symbol)")
    end

    it 'keeps elements returning nil from #to_ary' do
      x = Object.new
      def x.to_ary
        nil
      end

      [x].flatten.should == [x]
    end

    it "doesn't try #to_a" do
      x = Object.new
      def x.to_a
        :sym
      end

      [x].flatten.should == [x]
    end
  end

  describe 'permutation' do
    it 'returns all non-repeating permutations of the array' do
      a = [1, 2, 3]
      a.permutation.to_a.should == [[1, 2, 3], [1, 3, 2], [2, 1, 3], [2, 3, 1], [3, 1, 2], [3, 2, 1]]
      a.permutation(1).to_a.should == [[1], [2], [3]]
      a.permutation(2).to_a.should == [[1, 2], [1, 3], [2, 1], [2, 3], [3, 1], [3, 2]]
      a.permutation(3).to_a.should == [[1, 2, 3], [1, 3, 2], [2, 1, 3], [2, 3, 1], [3, 1, 2], [3, 2, 1]]
      a.permutation(0).to_a.should == [[]] # one permutation of length 0
      a.permutation(4).to_a.should == [] # no permutations of length 4
      a = ('a'..'e').to_a
      a.permutation(2).to_a.should == [
        %w[a b],
        %w[a c],
        %w[a d],
        %w[a e],
        %w[b a],
        %w[b c],
        %w[b d],
        %w[b e],
        %w[c a],
        %w[c b],
        %w[c d],
        %w[c e],
        %w[d a],
        %w[d b],
        %w[d c],
        %w[d e],
        %w[e a],
        %w[e b],
        %w[e c],
        %w[e d],
      ]
      a.permutation(3).to_a.should == [
        %w[a b c],
        %w[a b d],
        %w[a b e],
        %w[a c b],
        %w[a c d],
        %w[a c e],
        %w[a d b],
        %w[a d c],
        %w[a d e],
        %w[a e b],
        %w[a e c],
        %w[a e d],
        %w[b a c],
        %w[b a d],
        %w[b a e],
        %w[b c a],
        %w[b c d],
        %w[b c e],
        %w[b d a],
        %w[b d c],
        %w[b d e],
        %w[b e a],
        %w[b e c],
        %w[b e d],
        %w[c a b],
        %w[c a d],
        %w[c a e],
        %w[c b a],
        %w[c b d],
        %w[c b e],
        %w[c d a],
        %w[c d b],
        %w[c d e],
        %w[c e a],
        %w[c e b],
        %w[c e d],
        %w[d a b],
        %w[d a c],
        %w[d a e],
        %w[d b a],
        %w[d b c],
        %w[d b e],
        %w[d c a],
        %w[d c b],
        %w[d c e],
        %w[d e a],
        %w[d e b],
        %w[d e c],
        %w[e a b],
        %w[e a c],
        %w[e a d],
        %w[e b a],
        %w[e b c],
        %w[e b d],
        %w[e c a],
        %w[e c b],
        %w[e c d],
        %w[e d a],
        %w[e d b],
        %w[e d c],
      ]
      a.permutation(5).to_a.should == [
        %w[a b c d e],
        %w[a b c e d],
        %w[a b d c e],
        %w[a b d e c],
        %w[a b e c d],
        %w[a b e d c],
        %w[a c b d e],
        %w[a c b e d],
        %w[a c d b e],
        %w[a c d e b],
        %w[a c e b d],
        %w[a c e d b],
        %w[a d b c e],
        %w[a d b e c],
        %w[a d c b e],
        %w[a d c e b],
        %w[a d e b c],
        %w[a d e c b],
        %w[a e b c d],
        %w[a e b d c],
        %w[a e c b d],
        %w[a e c d b],
        %w[a e d b c],
        %w[a e d c b],
        %w[b a c d e],
        %w[b a c e d],
        %w[b a d c e],
        %w[b a d e c],
        %w[b a e c d],
        %w[b a e d c],
        %w[b c a d e],
        %w[b c a e d],
        %w[b c d a e],
        %w[b c d e a],
        %w[b c e a d],
        %w[b c e d a],
        %w[b d a c e],
        %w[b d a e c],
        %w[b d c a e],
        %w[b d c e a],
        %w[b d e a c],
        %w[b d e c a],
        %w[b e a c d],
        %w[b e a d c],
        %w[b e c a d],
        %w[b e c d a],
        %w[b e d a c],
        %w[b e d c a],
        %w[c a b d e],
        %w[c a b e d],
        %w[c a d b e],
        %w[c a d e b],
        %w[c a e b d],
        %w[c a e d b],
        %w[c b a d e],
        %w[c b a e d],
        %w[c b d a e],
        %w[c b d e a],
        %w[c b e a d],
        %w[c b e d a],
        %w[c d a b e],
        %w[c d a e b],
        %w[c d b a e],
        %w[c d b e a],
        %w[c d e a b],
        %w[c d e b a],
        %w[c e a b d],
        %w[c e a d b],
        %w[c e b a d],
        %w[c e b d a],
        %w[c e d a b],
        %w[c e d b a],
        %w[d a b c e],
        %w[d a b e c],
        %w[d a c b e],
        %w[d a c e b],
        %w[d a e b c],
        %w[d a e c b],
        %w[d b a c e],
        %w[d b a e c],
        %w[d b c a e],
        %w[d b c e a],
        %w[d b e a c],
        %w[d b e c a],
        %w[d c a b e],
        %w[d c a e b],
        %w[d c b a e],
        %w[d c b e a],
        %w[d c e a b],
        %w[d c e b a],
        %w[d e a b c],
        %w[d e a c b],
        %w[d e b a c],
        %w[d e b c a],
        %w[d e c a b],
        %w[d e c b a],
        %w[e a b c d],
        %w[e a b d c],
        %w[e a c b d],
        %w[e a c d b],
        %w[e a d b c],
        %w[e a d c b],
        %w[e b a c d],
        %w[e b a d c],
        %w[e b c a d],
        %w[e b c d a],
        %w[e b d a c],
        %w[e b d c a],
        %w[e c a b d],
        %w[e c a d b],
        %w[e c b a d],
        %w[e c b d a],
        %w[e c d a b],
        %w[e c d b a],
        %w[e d a b c],
        %w[e d a c b],
        %w[e d b a c],
        %w[e d b c a],
        %w[e d c a b],
        %w[e d c b a],
      ]
    end
  end

  describe '.[]' do
    it 'returns a new array populated with the given objects' do
      Array[].should == []
      Array.[](1, 'a', /^A/).should == [1, 'a', /^A/]
      Array[2, 'b', /^B/].should == [2, 'b', /^B/]
    end
  end

  describe '.new' do
    it 'returns an empty array given no args' do
      Array.new.should == []
    end

    it 'returns the array when given an array' do
      Array.new([1, 2, 3]).should == [1, 2, 3]
    end

    it 'returns an array of nils given a single integer arg' do
      Array.new(5).should == [nil, nil, nil, nil, nil]
    end

    it 'returns an array filled with the given value and given length' do
      Array.new(5, :foo).should == %i[foo foo foo foo foo]
    end

    it 'handles block correctly' do
      Array.new(2) { |i| i }.should == [0, 1]
    end
  end

  describe '#inspect' do
    it 'returns the syntax representation' do
      [1, 2, 3].inspect.should == '[1, 2, 3]'
    end
  end

  describe '#to_s' do
    it 'returns the syntax representation' do
      [1, 2, 3].to_s.should == '[1, 2, 3]'
    end
  end

  describe '<<' do
    it 'appends an item to the array' do
      a = [1, 2, 3]
      a << 4
      a.should == [1, 2, 3, 4]
    end
  end

  describe '+' do
    it 'returns a new copy of the given arrays' do
      a = [1, 2, 3] + [4, 5, 6]
      a.should == [1, 2, 3, 4, 5, 6]
    end
  end

  describe '-' do
    it 'returns a new copy of the array with elements removed' do
      a = [1, 2, 3, 4, 5, 6] - [1, 4, 5]
      a.should == [2, 3, 6]
      a = [1, 2, 3, 4, 5, 6] - []
      a.should == [1, 2, 3, 4, 5, 6]
    end
  end

  describe '&' do
    it 'should give empty array if either is empty' do
      ([] & []).should == []
      ([1, 2, 3] & []).should == []
      ([] & [1, 2, 3]).should == []
    end

    it 'should give an array with elements in both arrays' do
      ([1, 2, 3] & [1, 2, 3]).should == [1, 2, 3]
      ([1, 2, 3] & [1, 3, 2]).should == [1, 2, 3]
      ([1, 2, 3] & [2, 2, 3, 3, 1]).should == [1, 2, 3]

      ([1] & [2, 2, 3, 3, 1]).should == [1]
      ([3, 2] & [2, 2, 3, 3, 1]).should == [3, 2] # is the order supposed to be fixed?
      ([1, 2, 3] & [4, 5, 6, 7]).should == []
      ([1, 2, 3, -1] & [4, 5, 6, -1, 7]).should == [-1]
    end

    it 'should not modify the original arrays' do
      a = [1, 2, 3]
      b = [2, 4, 6]
      (a & b).should == [2]
      a.should == [1, 2, 3]
      b.should == [2, 4, 6]
    end

    it 'should preserve the order of the left array' do
      ([3, 2, 1] & [1, 2, 3]).should == [3, 2, 1]
      ([2, 1] & [1, 2, 3]).should == [2, 1]
      ([3, 2, 1] & [1, 2]).should == [2, 1]
    end

    it 'should throw on non array argument' do
      -> { [] & -3 }.should raise_error(TypeError, 'no implicit conversion of Integer into Array')
      -> { [] & :foo }.should raise_error(TypeError, 'no implicit conversion of Symbol into Array')
      -> { [] & 'a' }.should raise_error(TypeError, 'no implicit conversion of String into Array')
    end
  end

  describe '#intersection' do
    it 'should give a copy of self when no arguments given' do
      a = [1, 2, 3]
      b = a.intersection
      a.should == b
      a.should_not equal(b)
    end

    it 'should give an empty array if any array is empty' do
      [].intersection([1]).should == []
      [1, 2, 3].intersection([1], []).should == []
      [1, 2, 3].intersection([]).should == []
      [1, 2, 3].intersection([1, 2], [], []).should == []
      [1, 2, 3].intersection([], [1, 2], []).should == []
    end

    it 'should throw on non array arguments' do
      -> { [].intersection(-3, :foo) }.should raise_error(TypeError, 'no implicit conversion of Integer into Array')
      -> { [].intersection(:foo, -3) }.should raise_error(TypeError, 'no implicit conversion of Symbol into Array')
      -> { [].intersection('a', -3) }.should raise_error(TypeError, 'no implicit conversion of String into Array')
    end
  end

  describe '[]' do
    it 'returns the item at the given index' do
      a = [1, 2, 3, 4, 5, 6]
      a[0].should == 1
      a[1].should == 2
    end

    it 'returns the item offset from the end when given a negative index' do
      a = [1, 2, 3, 4, 5, 6]
      a[-1].should == 6
      a[-2].should == 5
    end

    it 'returns nil when the index is out of range' do
      a = [1, 2, 3, 4, 5, 6]
      a[10].should == nil
      a[-10].should == nil
    end

    it 'returns a sub-array when given a range' do
      a = [1, 2, 3, 4, 5, 6]
      a[0..3].should == [1, 2, 3, 4]
      a[0...3].should == [1, 2, 3]
      a[1..1].should == [2]
      a[1..5].should == [2, 3, 4, 5, 6]
      a[1..6].should == [2, 3, 4, 5, 6]
      a[1..10].should == [2, 3, 4, 5, 6]
      a[6..10].should == []
      a[6..10].should == []
      a[-2..-1].should == [5, 6]
      a[-1..-1].should == [6]
      a[-6..-1].should == [1, 2, 3, 4, 5, 6]
      a[-1..-6].should == []
      a[-10..-9].should == nil
      a[1..-1].should == [2, 3, 4, 5, 6]
      a[1...-1].should == [2, 3, 4, 5]
      a[1...1].should == []
      a[-1...-1].should == []
      a = []
      a[0..-2].should == []
    end

    it 'should return nil when given range starting after length of array' do
      a = [1, 2, 3]
      a[4..5].should == nil
      a[4..3].should == nil
      a[4..2].should == nil
      a[4..-1].should == nil
      a[-10..-1].should == nil

      b = []
      b[0..100].should == []
      b[-0..100].should == []
      b[1..100].should == nil
      b[-1..100].should == nil
    end

    it 'should throw an error on unknown argument types' do
      a = [1, 2, 3]
      -> { a['a'] }.should raise_error(TypeError, 'no implicit conversion of String into Integer')
      -> { a['a', 'b'] }.should raise_error(TypeError, 'no implicit conversion of String into Integer')
      -> { a['a'..'c'] }.should raise_error(TypeError, 'no implicit conversion of String into Integer')
      -> { a[:foo] }.should raise_error(TypeError, 'no implicit conversion of Symbol into Integer')
      -> { a[:foo, :bar] }.should raise_error(TypeError, 'no implicit conversion of Symbol into Integer')
      -> { a[:foo..:bar] }.should raise_error(TypeError, 'no implicit conversion of Symbol into Integer')
      -> { a[[]] }.should raise_error(TypeError, 'no implicit conversion of Array into Integer')
      -> { a[[1]] }.should raise_error(TypeError, 'no implicit conversion of Array into Integer')
      -> { a[nil] }.should raise_error(TypeError, 'no implicit conversion from nil to integer')
      -> { a[1, nil] }.should raise_error(TypeError, 'no implicit conversion from nil to integer')
    end
  end

  describe '[]=' do
    context 'given a single number argument' do
      it 'replaces the item at the given index' do
        a = [1, 2, 3, 4, 5, 6]
        a[1] = 'two'
        a.should == [1, 'two', 3, 4, 5, 6]
      end

      it 'fills the array with nils when the index is larger than array' do
        a = [1, 2, 3, 4, 5, 6]
        a[10] = 11
        a.should == [1, 2, 3, 4, 5, 6, nil, nil, nil, nil, 11]
      end

      it 'should start from the back for a negative number' do
        a = [1, 2, 3]
        a[-1] = 11
        a.should == [1, 2, 11]
      end

      it 'should raise an error if negative index before beginning' do
        a = [1, 2, 3]
        -> { a[-4] = :foo }.should raise_error(IndexError, 'index -4 too small for array; minimum: -3')
        -> { a[-100] = :foo }.should raise_error(IndexError, 'index -100 too small for array; minimum: -3')
      end

      it 'should extend and put nil' do
        a = [1, 2, 3]
        a[4] = nil
        a.should == [1, 2, 3, nil, nil]

        a[0] = nil
        a.should == [nil, 2, 3, nil, nil]
      end

      it 'should not all items of an array as rhs but as array' do
        a = [1, 2, 3]
        a[1] = [9, 10]
        a.should == [1, [9, 10], 3]
      end
    end

    context 'given two number arguments' do
      it 'should replace the item at the start position and remove all the ones after' do
        a = [1, 2, 3]
        (a[0, 2] = 'a').should == 'a'
        a.should == ['a', 3]
      end

      it 'should insert a new item for length = 0' do
        a = [1, 2, 3]
        (a[1, 0] = 'x').should == 'x'
        a.should == [1, 'x', 2, 3]
      end

      it 'should not extend the array if start + size beyond array size' do
        a = [1, 2, 3]
        a[2, 6] = 99
        a.should == [1, 2, 99]
      end

      it 'should start from the back given a negative start' do
        a = [1, 2, 3]
        a[-2, 6] = 99
        a.should == [1, 99]
        a[-2, 0] = 101
        a.should == [101, 1, 99]
      end

      it 'should extend and put nil' do
        a = [1, 2, 3]
        a[4, 2] = nil
        a.should == [1, 2, 3, nil, nil]

        a[0, 2] = nil
        a.should == [nil, 3, nil, nil]
      end

      it 'should add all items of an separately for rhs array' do
        a = [1, 2, 3]
        a[1, 2] = [9, 10]
        a.should == [1, 9, 10]

        a = [1, 2, 3, 4]
        a[1, 1] = [11, 12]
        a.should == [1, 11, 12, 3, 4]
      end

      it 'should raise an error on negative length' do
        a = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        -> { a[-2, -1] = 99 }.should raise_error(IndexError, 'negative length (-1)')
        -> { a[-2, -3] = 99 }.should raise_error(IndexError, 'negative length (-3)')
        -> { a[0, -4] = 99 }.should raise_error(IndexError, 'negative length (-4)')
        -> { a[100, -5] = 99 }.should raise_error(IndexError, 'negative length (-5)')
        -> { a[1, -6] = 99 }.should raise_error(IndexError, 'negative length (-6)')
      end
    end

    context 'given a range' do
      it 'should remove all items in range and add value add start of range' do
        a = [1, 2, 3]
        a[2..3] = :foo
        a.should == [1, 2, :foo]

        a[1..1] = 'a'
        a.should == [1, 'a', :foo]
      end

      it 'should only insert element if start and end are size of array' do
        a = []
        a[0...0] = 1
        a.should == [1]
        a[1...1] = '2'
        a.should == [1, '2']

        a[1...1] = :onepointfive
        a.should == [1, :onepointfive, '2']
      end

      it 'should replace items if not excluding end and within array size' do
        a = [1, 2, 3]
        a[1..1] = :nottwo
        a.should == [1, :nottwo, 3]
      end

      it 'should insert items when range is excluding end and start and end the same' do
        a = [1, 2, 3]
        a[1...1] = '1.5'
        a.should == [1, '1.5', 2, 3]
        a[-1...-1] = '2.5'
        a.should == [1, '1.5', 2, '2.5', 3]
      end

      it 'should not extend the array past the original size even if end is larger' do
        a = [1, 2, 3]
        a[2..100] = 4
        a.should == [1, 2, 4]
      end

      it 'should only extend the array until start and not end of range' do
        a = [1, 2, 3]
        a[5..100] = 'x'
        a.should == [1, 2, 3, nil, nil, 'x']
      end

      it 'should replace only the items at negative start when range ends with negative' do
        a = [1, 2, 3]
        a[-1..-5] = 4
        a.should == [1, 2, 4, 3]
      end

      it 'should replace the items if range goes from negative to positive' do
        a = [1, 2, 3]
        a[-2..3] = :foo
        a.should == [1, :foo]

        # end before start (after conversion)
        a = [1, 2, 3, 4]
        a[-2..1] = :foo
        a.should == [1, 2, :foo, 3, 4]
      end

      it 'should add all items of an separately for rhs array' do
        a = [1, 2, 3]
        a[1..2] = [9, 10]
        a.should == [1, 9, 10]

        a = [1, 2, 3, 4]
        a[1..1] = [11, 12]
        a.should == [1, 11, 12, 3, 4]
      end

      it 'should?' do
        a = [1, 2, 3]
        -> { a[-4..-6] = 3 }.should raise_error(RangeError, '-4..-6 out of range')
        -> { a[-4..0] = 3 }.should raise_error(RangeError, '-4..0 out of range')
        -> { a[-4..2] = 3 }.should raise_error(RangeError, '-4..2 out of range')
        -> { a[-4...-6] = 3 }.should raise_error(RangeError, '-4...-6 out of range')
        -> { a[-4...0] = 3 }.should raise_error(RangeError, '-4...0 out of range')
        -> { a[-4...2] = 3 }.should raise_error(RangeError, '-4...2 out of range')

        -> { a[-100...100] = 3 }.should raise_error(RangeError, '-100...100 out of range')
      end
    end

    it 'should throw an error on unknown argument types' do
      a = [1, 2, 3]
      -> { a['a'] = 1 }.should raise_error(TypeError, 'no implicit conversion of String into Integer')
      -> { a['a', 'b'] = 'd' }.should raise_error(TypeError, 'no implicit conversion of String into Integer')
      -> { a['a'..'c'] = :a }.should raise_error(TypeError, 'no implicit conversion of String into Integer')
      -> { a[:foo] = 'v' }.should raise_error(TypeError, 'no implicit conversion of Symbol into Integer')
      -> { a[:foo, :bar] = 'id' }.should raise_error(TypeError, 'no implicit conversion of Symbol into Integer')
      -> { a[:foo..:bar] = 0 }.should raise_error(TypeError, 'no implicit conversion of Symbol into Integer')
      -> { a[:foo...:bar] = 't' }.should raise_error(TypeError, 'no implicit conversion of Symbol into Integer')
      -> { a[[]] = [1, 2] }.should raise_error(TypeError, 'no implicit conversion of Array into Integer')
      -> { a[[1]] = [3, 4] }.should raise_error(TypeError, 'no implicit conversion of Array into Integer')

      -> { a[nil] = [3, 4] }.should raise_error(TypeError, 'no implicit conversion from nil to integer')
      -> { a[nil] = nil }.should raise_error(TypeError, 'no implicit conversion from nil to integer')
      -> { a[nil, nil] = nil }.should raise_error(TypeError, 'no implicit conversion from nil to integer')
      -> { a[3, nil] = nil }.should raise_error(TypeError, 'no implicit conversion from nil to integer')
      -> { a[nil, 3] = nil }.should raise_error(TypeError, 'no implicit conversion from nil to integer')
      -> { a[nil, nil] = 'a' }.should raise_error(TypeError, 'no implicit conversion from nil to integer')
      -> { a[3, nil] = :foo }.should raise_error(TypeError, 'no implicit conversion from nil to integer')
      -> { a[nil, 3] = -5 }.should raise_error(TypeError, 'no implicit conversion from nil to integer')
    end
  end

  describe '#size' do
    it 'returns the array size' do
      [].size.should == 0
      [1, 2, 3, 4, 5, 6].size.should == 6
    end
  end

  describe '#length' do
    it 'returns the array size' do
      [].length.should == 0
      [1, 2, 3, 4, 5, 6].length.should == 6
    end
  end

  describe '#any?' do
    context 'given no args' do
      it 'returns true if the array has any items' do
        [].any?.should == false
        [1, 2, 3, 4, 5, 6].any?.should == true
      end
    end

    context 'given block' do
      it 'should use the condition of the block' do
        [1, 2, 3].any? { |i| i == 2 }.should == true
        [1, 2, 3].any? { |i| i == 4 }.should == false
      end
    end
  end

  describe '==' do
    it 'returns true if the two arrays are equal' do
      ([] == [1]).should == false
      ([] == []).should == true
      ([1, 2, 3] == [1, 2, 3]).should == true
      ([1, 2, 3] == [3, 2, 1]).should == false
    end

    it 'does not return true always for recursive arrays' do
      a = []
      a << a
      a.should_not == [[2]]
    end
  end

  describe 'eql?' do
    it 'does not return true always for recursive arrays' do
      a = []
      a << a
      a.should_not eql [[2]]
    end
  end

  describe '#each' do
    it 'evaluates the block for each item in the array' do
      result = []
      [1, 2, 3].each { |i| result << i * 2 }
      result.should == [2, 4, 6]
    end
  end

  describe '#each_with_index' do
    it 'evaluates the block for each item in the array, passing along an index too' do
      result = []
      %w[a b c].each_with_index { |c, i| result << [c, i] }
      result.should == [['a', 0], ['b', 1], ['c', 2]]
    end

    it 'returns an enumerator' do
      enum = %w[a b c].each_with_index
      enum.should be_an_instance_of(Enumerator)
      enum.to_a.should == [['a', 0], ['b', 1], ['c', 2]]
    end
  end

  describe '#map' do
    it 'returns a new array of the result of evaluating the block for each item in the array' do
      result = [1, 2, 3].map { |i| i * 2 }
      result.should == [2, 4, 6]
    end
  end

  describe '#collect' do
    it 'returns a new array of the result of evaluating the block for each item in the array' do
      result = [1, 2, 3].collect { |i| i * 2 }
      result.should == [2, 4, 6]
    end
  end

  describe '#first' do
    it 'returns the first item in the array' do
      [1, 2, 3].first.should == 1
    end

    it 'returns nil if the array is empty' do
      [].first.should == nil
    end
  end

  describe '#last' do
    it 'returns the last item in the array' do
      [1, 2, 3].last.should == 3
    end

    it 'returns nil if the array is empty' do
      [].last.should == nil
    end
  end

  describe '#first(n)' do
    it 'should return a new array of the first n elements' do
      [1, 2, 3].first(1).should == [1]
      [1, 2, 3].first(2).should == [1, 2]
      [1, 2, 3].first(3).should == [1, 2, 3]
      %w[a b].first(1).should == ['a']
    end

    it 'returns an empty array if n == 0' do
      [1, 2, 3].first(0).should == []
      [].first(0).should == []
    end

    it 'returns empty array if the array is empty' do
      [].first(2).should == []
    end

    it 'Should raise ArgumentError on negative n' do
      -> { [].first(-1) }.should raise_error(ArgumentError, 'negative array size')
    end
  end

  describe '#last(n)' do
    it 'should return a new array of the last n elements' do
      [1, 2, 3].last(1).should == [3]
      [1, 2, 3].last(2).should == [2, 3]
      [1, 2, 3].last(3).should == [1, 2, 3]
      %w[a b].last(1).should == ['b']
    end

    it 'returns an empty array if n <= 0' do
      [1, 2, 3].last(0).should == []
      [].last(0).should == []
    end

    it 'returns empty array if the array is empty' do
      [].last(2).should == []
    end

    it 'Should raise ArgumentError on negative n' do
      -> { [].last(-1) }.should raise_error(ArgumentError, 'negative array size')
    end
  end

  describe '#drop(n)' do
    it 'should return the same array for drop 0' do
      a = []
      a.drop(0).should == []
      a.should == []

      a = [1, 2, 3]
      a.drop(0).should == [1, 2, 3]
      a.should == [1, 2, 3]
    end

    it 'should throw error on negative index' do
      -> { [].drop(-1) }.should raise_error(ArgumentError, 'attempt to drop negative size')
      -> { [1, 2, 3].drop(-2) }.should raise_error(ArgumentError, 'attempt to drop negative size')
    end

    it 'should return the array with the first n elements removed' do
      a = [1, 2, 3]
      a.drop(1).should == [2, 3]
      a.drop(2).should == [3]
      a.drop(3).should == []
      a.should == [1, 2, 3]
    end

    it 'should not throw if dropping more items than array size' do
      a = [1, 2, 3]
      a.drop(4).should == []
      a.drop(5).should == []
      a.drop(1000).should == []
      a.should == [1, 2, 3]
    end
  end

  describe '#to_ary' do
    it 'returns itself' do
      [1, 2, 3].to_ary.should == [1, 2, 3]
    end
  end

  describe '#pop' do
    it 'removes the last item and returns it' do
      a = [1, 2, 3]
      a.pop.should == 3
      a.should == [1, 2]
    end

    it 'returns nil if the array is empty' do
      a = []
      a.pop.should == nil
      a.should == []
    end
  end

  describe '#include?' do
    it 'returns true if the given item is present in the array' do
      [1, 2, 3].include?(3).should == true
      [1, 2, 3].include?(4).should == false
      [].include?(4).should == false
    end
  end

  describe '#sort' do
    it 'returns a sorted copy of the array' do
      a = [5, 3, 2, 1, 7, 9]
      a2 = a.sort
      a2.should == [1, 2, 3, 5, 7, 9]
      a.should == [5, 3, 2, 1, 7, 9]
      %w[a z c foo bar].sort.should == %w[a bar c foo z]
    end
  end

  describe '#join' do
    it 'returns an empty string for an empty array' do
      [].join(',').should == ''
    end

    it 'returns the only element in a 1-element array' do
      ['foo'].join(',').should == 'foo'
      [1].join(',').should == '1'
    end

    it 'returns the items joined together in a string' do
      %w[foo bar].join('').should == 'foobar'
      %w[foo bar].join(',').should == 'foo,bar'
      [:foo, 2, nil, 'bar', { baz: :buz }].join(',').should == 'foo,2,,bar,{:baz=>:buz}'
    end

    it 'does not mutate anything' do
      a = %w[foo bar]
      a.join(' ')
      a.should == %w[foo bar]
    end
  end

  describe '#select' do
    it 'returns a new array containing items for which the block yielded true' do
      [1, 2, 3, 4].select { |i| i % 2 == 0 }.should == [2, 4]
    end

    it 'returns an enumerator when no block is given' do
      e = [1, 2, 3, 4].select
      e.should be_an_instance_of(Enumerator)
      e.each { |i| i <= 2 }.should == [1, 2]
    end
  end

  describe '#reject' do
    it 'returns a new array containing items for which the block yielded false' do
      [1, 2, 3, 4].reject { |i| i % 2 == 0 }.should == [1, 3]
    end

    it 'returns an enumerator when no block is given' do
      e = [1, 2, 3, 4].reject
      e.should be_an_instance_of(Enumerator)
      e.each { |i| i <= 2 }.should == [3, 4]
    end
  end

  describe '#shift' do
    it 'works' do
      a = [1, 2, 3, 4, 5]
      a.shift.should == 1
      a.shift(2).should == [2, 3]
      a.shift(0).should == []
      a.should == [4, 5]
    end
  end

  describe '#sample' do
    it 'works' do
      a = [1, 2, 3]

      occurrences = []

      1000.times do
        sample = a.sample

        occurrences << sample unless occurrences.include?(sample)
      end

      a.each { |e| occurrences.include?(e).should == true }
    end

    it 'returns nil for empty arrays' do
      [].sample.should == nil
    end
  end

  describe '#max' do
    specify do
      [].max.should == nil
      [10].max.should == 10
      [1, 3].max.should == 3
      [1, 0, -1].max.should == 1
      %w[abc mno xyz].max.should == 'xyz'
    end
  end

  describe '#min' do
    specify do
      [].min.should == nil
      [10].min.should == 10
      [1, 3].min.should == 1
      [1, 0, -1].min.should == -1
      %w[abc mno xyz].min.should == 'abc'
    end
  end

  describe '#clear' do
    it 'should leave an empty array empty' do
      a = []
      a.clear.should equal(a)
      a.should == []
    end

    it 'should clear a non empty array' do
      a = [1, 2, 'bar']
      a.clear.should equal(a)
      a.should == []
    end
  end

  describe '#at' do
    it 'should return the item at the given index' do
      a = [:foo, 'bar', 2]
      a.at(0).should == :foo
      a.at(1).should == 'bar'
      a.at(2).should == 2
    end

    it 'negative index should give elements from the back' do
      a = [1, 2, 3]
      a.at(-1).should == 3
      a.at(-2).should == 2
      a.at(-3).should == 1
    end

    it 'should return nil on index out of bounds' do
      a = [0]
      a.at(1).should == nil
      a.at(2).should == nil
      a.at(-2).should == nil
      a = []
      a.at(0).should == nil
      a.at(-1).should == nil
    end
  end

  describe '#assoc' do
    it 'should return nil on empty array' do
      a = []
      a.assoc(0).should == nil
    end

    it 'should return nil on array without array with matching first element' do
      a = [1, 2, [3, 4], [5, 6]]
      a.assoc(0).should == nil
      a.assoc(1).should == nil
      a.assoc(4).should == nil
    end

    it 'should return the array matching the first element' do
      a = [[1]]
      a.assoc(1).should == [1]
    end

    it 'should return the first array with matching first element' do
      a = [[2, 1], [1, 2], [2, 1, 3], [1, 3]]
      a.assoc(1).should == [1, 2]
      a.assoc(2).should == [2, 1]
    end
  end

  describe '#rassoc' do
    it 'should return nil on empty array' do
      a = []
      a.rassoc(0).should == nil
    end

    it 'should return nil on array without array with matching first element' do
      a = [1, 2, [3, 4], [5, 6]]
      a.rassoc(0).should == nil
      a.rassoc(1).should == nil
      a.rassoc(3).should == nil
    end

    it 'should return the array matching the first element' do
      a = [[2, 1]]
      a.rassoc(1).should == [2, 1]
    end

    it 'should return the first array with matching first element' do
      a = [[2, 1], [1, 2], [2, 1, 3], [1, 2, 3]]
      a.rassoc(1).should == [2, 1]
      a.rassoc(2).should == [1, 2]
    end
  end

  describe '#compact' do
    specify do
      [].compact.should == []
      [nil, 1, nil].compact.should == [1]
      [nil, false].compact.should == [false]
      a = []
      a.compact.object_id.should_not == a.object_id
    end
  end

  describe '#push' do
    specify do
      a = []
      a.push
      a.should == []
      a.push(1, 2, 3)
      a.should == [1, 2, 3]
      a.push(4)
      a.should == [1, 2, 3, 4]
    end
  end

  describe '#pack' do
    describe 'c' do
      it 'handle bignums' do
        [fixnum_max + 1].pack('c').should == "\x00".force_encoding(Encoding::BINARY)
        [fixnum_max + 2].pack('c').should == "\x01".force_encoding(Encoding::BINARY)
        [-fixnum_max - 1].pack('c').should == "\x00".force_encoding(Encoding::BINARY)
        [-fixnum_max - 2].pack('c').should == "\xFF".force_encoding(Encoding::BINARY)
      end
    end
  end

  describe '#append' do
    specify do
      a = []
      a.append
      a.should == []
      a.append(1, 2, 3)
      a.should == [1, 2, 3]
      a.append(4)
      a.should == [1, 2, 3, 4]
    end
  end

  describe '#index' do
    specify do
      a = [1, 2, 3]
      a.index(2).should == 1
      a = %w[a b c]
      a.index('c').should == 2
      a.index('d').should == nil
      a.index(nil).should == nil
      a.index { |i| i == 'a' }.should == 0
      # TODO
      #a.index.should be_an_instance_of(Enumerator)
      #a.index.each { |i| i == 'c' }.should == 2
    end

    it 'should return the first occurrence of object' do
      a = [1, 2, 2, 3, 4, 1]
      a.index(1).should == 0
      a.index(2).should == 1
      a.index { |i| i == 3 or i == 4 }.should == 3
    end
  end

  describe '#find_index' do
    specify do
      a = [1, 2, 3]
      a.find_index(2).should == 1
      a = %w[a b c]
      a.find_index('c').should == 2
      a.find_index('d').should == nil
      a.find_index(nil).should == nil
      a.find_index { |i| i == 'a' }.should == 0
      # TODO
      #a.find_index.should be_an_instance_of(Enumerator)
      #a.find_index.each { |i| i == 'c' }.should == 2
    end

    it 'should return the first occurrence of object' do
      a = [1, 2, 2, 3, 4, 1]
      a.find_index(1).should == 0
      a.find_index(2).should == 1
      a.find_index { |i| i == 3 or i == 4 }.should == 3
    end
  end

  describe '#uniq' do
    specify do
      [1, 1, 2, 1, 3, 3, 1.0].uniq.should == [1, 2, 3, 1.0]
      %w[a b b a].uniq.should == %w[a b]
      o1 = Object.new
      o2 = Object.new
      [o1, o1, o2].uniq.should == [o1, o2]
    end
  end

  describe '#reverse' do
    it 'should give the same array for empty or single element' do
      [].reverse.should == []
      [1].reverse.should == [1]
      [:foo].reverse.should == [:foo]
    end

    it 'should swap the elements for arrays of size 2' do
      [1, 2].reverse.should == [2, 1]
      [2, 1].reverse.should == [1, 2]
    end

    it 'should reverse the entire array' do
      [1, 2, 3].reverse.should == [3, 2, 1]
      [1, 2, 3, 4].reverse.should == [4, 3, 2, 1]
      [1, 2, 3, 4, 5].reverse.should == [5, 4, 3, 2, 1]
    end

    it 'should not reverse the original array' do
      a = [1, 2, 3]
      a.reverse.should == [3, 2, 1]
      a.should == [1, 2, 3]
    end
  end

  describe '#reverse!' do
    it 'should give the same array for empty and singleton array' do
      a = []
      a.reverse!.should equal(a)
      a = [1]
      a.reverse!.should equal(a)
      a.should == [1]
    end

    it 'should reverse the array in place' do
      a = [1, 2, 3]
      a.reverse!.should equal(a)
      a.should == [3, 2, 1]

      a = [1, 2, 3, 4]
      a.reverse!.should equal(a)
      a.should == [4, 3, 2, 1]
    end
  end

  describe '#concat' do
    it 'should give itself even with no arguments' do
      a = [1, 2, 3]
      b = a.concat
      a.should equal(b)
      b.should == [1, 2, 3]
    end

    it 'should give itself and not a copy when concatenating' do
      a = [1, 2]
      a.concat([3, 4]).should equal(a)
      a.should == [1, 2, 3, 4]
    end

    it 'should give the concatonation of two arrays' do
      [1, 2].concat([3, 4]).should == [1, 2, 3, 4]
      [].concat([1, 2]).should == [1, 2]
      [3, 4].concat([]).should == [3, 4]
    end

    it 'should concatenate all the arrays' do
      [1].concat([999], [3, 4], [5, [6, 7]]).should == [1, 999, 3, 4, 5, [6, 7]]
      [:foo].concat([:bar], [3], ['c']).should == [:foo, :bar, 3, 'c']
    end

    it 'should raise error on non array argument' do
      -> { [1].concat([2], 3) }.should raise_error(TypeError, 'no implicit conversion of Integer into Array')
      -> { [1].concat([2], :foo) }.should raise_error(TypeError, 'no implicit conversion of Symbol into Array')
      -> { [1].concat([2], 'a') }.should raise_error(TypeError, 'no implicit conversion of String into Array')
    end
  end

  describe '#rindex' do
    specify do
      a = [1, 2, 3]
      a.rindex(2).should == 1
      a = %w[a b c]
      a.rindex('c').should == 2
      a.rindex('d').should == nil
      a.rindex(nil).should == nil
      a.rindex { |i| i == 'a' }.should == 0
      # TODO
      #a.index.should be_an_instance_of(Enumerator)
      #a.index.each { |i| i == 'c' }.should == 2
    end

    it 'should return the first occurrence of object' do
      a = [1, 2, 2, 3, 4, 1]
      a.rindex(1).should == 5
      a.rindex(2).should == 2
      a.rindex { |i| i == 3 or i == 4 }.should == 4
    end
  end

  describe '#none?' do
    it 'returns true if the array has no items' do
      [].none?.should == true
    end

    it 'returns true if the array has only falsy items' do
      [nil, false].none?.should == true
    end

    it 'returns false if any item is truthy' do
      [1].none?.should == false
      [:foo].none?.should == false
      [[]].none?.should == false
      ['c'].none?.should == false
    end

    it 'should support block based condition' do
      [1, 2, 3].none? { |i| i == 2 }.should == false
      [1, 2, 3].none? { |i| i == 4 }.should == true
    end
  end

  describe '#one?' do
    it 'returns false if the array has no items' do
      [].one?.should == false
    end

    it 'returns false if the array has only falsy items' do
      [nil, false].one?.should == false
      [nil].one?.should == false
      [nil, false, nil, false, nil, false].one?.should == false
    end

    it 'returns true if the array has exactly one item which is truthy' do
      [1].one?.should == true
      [1, 2].one?.should == false

      [:foo].one?.should == true
      %i[foo baar].one?.should == false

      [[]].one?.should == true
      [[], []].one?.should == false

      ['c'].one?.should == true
      %w[c z].one?.should == false
    end

    it 'should support block based condition' do
      [1, 2, 3].one? { |i| i == 2 }.should == true
      [1, 2, 3].one? { |i| i >= 2 }.should == false
    end
  end

  describe '#union' do
    it 'should give a copy of self when no arguments given' do
      a = [1, 2, 3]
      b = a.union
      a.should == b
      a.should_not equal(b)
    end

    it 'should give empty array only if both empty' do
      [].union([]).should == []
      [].union([], []).should == []
      [].union([], [], []).should == []
    end

    it 'should give an array with elements in either arrays' do
      [1, 2].union([3, 4]).should == [1, 2, 3, 4]
      [].union([1, 2, 3]).should == [1, 2, 3]
      [3, 1, 2].union([]).should == [3, 1, 2]

      [].union([], [1], [1], [1, 2], [], [2]).should == [1, 2]
    end

    it 'should preserve order' do
      [1, 2].union([3, 4]).should == [1, 2, 3, 4]
      [3, 4].union([1, 2]).should == [3, 4, 1, 2]
      [0].union([1], [2], [3]).should == [0, 1, 2, 3]

      [0].union([2], [1], [1], [1, 2], [], [2]).should == [0, 2, 1]
    end

    it 'should throw on non array arguments' do
      -> { [].union(-3, :foo) }.should raise_error(TypeError, 'no implicit conversion of Integer into Array')
      -> { [].union(:foo, -3) }.should raise_error(TypeError, 'no implicit conversion of Symbol into Array')
      -> { [].union('a', -3) }.should raise_error(TypeError, 'no implicit conversion of String into Array')
    end
  end

  describe '#|' do
    it 'should give empty array only if both empty' do
      ([] | []).should == []
    end

    it 'should give an array with elements in either array' do
      ([1, 2] | [3, 4]).should == [1, 2, 3, 4]
      ([] | [1, 2, 3]).should == [1, 2, 3]
      ([3, 1, 2] | []).should == [3, 1, 2]
    end

    it 'should not duplicate elements' do
      ([1, 1, 1, 1, 1, 1, 1] | [2, 2, 2, 2, 2, 2, 1, 2]).should == [1, 2]
      ([:foo, 'bar', 3] | [:foo, 'bar', 3, :foo, 'bar', 3, :foo, 'bar', 3]).should == [:foo, 'bar', 3]
    end

    it 'should not modify the original array' do
      a = [1, 2]
      b = [2, 4]
      (a | b).should == [1, 2, 4]
      a.should == [1, 2]
      b.should == [2, 4]
    end

    it 'should preserve order' do
      ([1, 2] | [3, 4]).should == [1, 2, 3, 4]
      ([3, 4] | [1, 2]).should == [3, 4, 1, 2]
    end

    it 'should throw on non array argument' do
      -> { [] | -3 }.should raise_error(TypeError, 'no implicit conversion of Integer into Array')
      -> { [] | :foo }.should raise_error(TypeError, 'no implicit conversion of Symbol into Array')
      -> { [] | 'a' }.should raise_error(TypeError, 'no implicit conversion of String into Array')
    end
  end

  describe '#rotate' do
    context 'given no arguments' do
      it 'should return an new empty array when called on empty' do
        [].rotate.should == []

        a = []
        a.rotate.should_not equal(a)
      end

      it 'should not modify the original array' do
        a = [1, 2]
        b = a.rotate
        a.should == [1, 2]
        b.should == [2, 1]
      end

      it 'should rotate the array' do
        [1].rotate.should == [1]
        [1, 2].rotate.should == [2, 1]
        [1, :foo, 'bar', 4].rotate.should == [:foo, 'bar', 4, 1]
      end
    end

    context 'given an argument' do
      it 'should return a copy of the original array when count = 0' do
        a = [1, 2, 3]
        b = a.rotate(0)
        b.should_not equal(a)
        b.should == a
      end

      it 'should rotate the array count amount of times' do
        [1, 2, 3].rotate(2).should == [3, 1, 2]
        [:foo, 'bar', 3, 2].rotate(2).should == [3, 2, :foo, 'bar']
      end

      it 'should handle larger than or equal to size rotations' do
        [1, 2, 3].rotate(3).should == [1, 2, 3]
        [1, 2, 3].rotate(4).should == [2, 3, 1]
        [1, 2, 3].rotate(5).should == [3, 1, 2]
        [1, 2, 3].rotate(3000).should == [1, 2, 3]
        [1, 2, 3].rotate(3001).should == [2, 3, 1]
      end

      it 'should handle negative numbers as right rotations' do
        [1, 2, 3].rotate(-0).should == [1, 2, 3]
        [1, 2, 3].rotate(-1).should == [3, 1, 2]
        [1, 2, 3].rotate(-2).should == [2, 3, 1]
        [1, 2, 3].rotate(-3).should == [1, 2, 3]
        [1, 2, 3].rotate(-4).should == [3, 1, 2]
      end

      it 'should return a copy even with negative count' do
        a = [1, 2, 3]
        b = a.rotate(-2)
        b.should_not equal(a)
        b.should == [2, 3, 1]
        a.should == [1, 2, 3]
      end

      it 'should raise an error if non number argument' do
        -> { [].rotate(:foo) }.should raise_error(TypeError, 'no implicit conversion of Symbol into Integer')
        -> { [].rotate([]) }.should raise_error(TypeError, 'no implicit conversion of Array into Integer')
        -> { [].rotate('a') }.should raise_error(TypeError, 'no implicit conversion of String into Integer')
      end
    end
  end

  describe '#rotate!' do
    context 'given no arguments' do
      it 'should return itself even when called on empty' do
        a = []
        a.rotate!.should equal(a)
        a.should == []
      end

      it 'should modify the original array' do
        a = [1, 2]
        b = a.rotate!
        a.should == [2, 1]
        b.should equal(a)
      end

      it 'should rotate the array' do
        [1].rotate!.should == [1]
        [1, 2].rotate!.should == [2, 1]
        [1, :foo, 'bar', 4].rotate!.should == [:foo, 'bar', 4, 1]
      end
    end

    context 'given an argument' do
      it 'should return the original array when count = 0' do
        a = [1, 2, 3]
        b = a.rotate!(0)
        b.should equal(a)
        a.should == [1, 2, 3]
      end

      it 'should rotate the array count amount of times' do
        [1, 2, 3].rotate!(2).should == [3, 1, 2]
        [:foo, 'bar', 3, 2].rotate!(2).should == [3, 2, :foo, 'bar']
      end

      it 'should handle larger than or equal to size rotations' do
        [1, 2, 3].rotate!(3).should == [1, 2, 3]
        [1, 2, 3].rotate!(4).should == [2, 3, 1]
        [1, 2, 3].rotate!(5).should == [3, 1, 2]
        [1, 2, 3].rotate!(3000).should == [1, 2, 3]
        [1, 2, 3].rotate!(3001).should == [2, 3, 1]
      end

      it 'should handle negative numbers as right rotations' do
        [1, 2, 3].rotate!(-0).should == [1, 2, 3]
        [1, 2, 3].rotate!(-1).should == [3, 1, 2]
        [1, 2, 3].rotate!(-2).should == [2, 3, 1]
        [1, 2, 3].rotate!(-3).should == [1, 2, 3]
        [1, 2, 3].rotate!(-4).should == [3, 1, 2]
      end

      it 'should modify self even with negative count' do
        a = [1, 2, 3]
        a.rotate!(-2).should equal(a)
        a.should == [2, 3, 1]
      end

      it 'should raise an error if non number argument' do
        -> { [].rotate!(:foo) }.should raise_error(TypeError, 'no implicit conversion of Symbol into Integer')
        -> { [].rotate!([]) }.should raise_error(TypeError, 'no implicit conversion of Array into Integer')
        -> { [].rotate!('a') }.should raise_error(TypeError, 'no implicit conversion of String into Integer')
      end
    end
  end

  describe '#slice' do
    it 'returns the item at the given index' do
      a = [1, 2, 3, 4, 5, 6]
      a.slice(0).should == 1
      a.slice(1).should == 2
    end

    it 'returns the item offset from the end when given a negative index' do
      a = [1, 2, 3, 4, 5, 6]
      a.slice(-1).should == 6
      a.slice(-2).should == 5
    end

    it 'returns nil when the index is out of range' do
      a = [1, 2, 3, 4, 5, 6]
      a.slice(10).should == nil
      a.slice(-10).should == nil
    end

    it 'returns a sub-array when given a range' do
      a = [1, 2, 3, 4, 5, 6]
      a.slice(0..3).should == [1, 2, 3, 4]
      a.slice(0...3).should == [1, 2, 3]
      a.slice(1..1).should == [2]
      a.slice(1..5).should == [2, 3, 4, 5, 6]
      a.slice(1..6).should == [2, 3, 4, 5, 6]
      a.slice(1..10).should == [2, 3, 4, 5, 6]
      a.slice(6..10).should == []
      a.slice(6..10).should == []
      a.slice(-2..-1).should == [5, 6]
      a.slice(-1..-1).should == [6]
      a.slice(-6..-1).should == [1, 2, 3, 4, 5, 6]
      a.slice(-1..-6).should == []
      a.slice(-10..-9).should == nil
      a.slice(1..-1).should == [2, 3, 4, 5, 6]
      a.slice(1...-1).should == [2, 3, 4, 5]
      a.slice(1...1).should == []
      a.slice(-1...-1).should == []
      a = []
      a.slice(0..-2).should == []
    end

    it 'should return nil when given range starting after length of array' do
      a = [1, 2, 3]
      a.slice(4..5).should == nil
      a.slice(4..3).should == nil
      a.slice(4..2).should == nil
      a.slice(4..-1).should == nil
      a.slice(-10..-1).should == nil

      b = []
      b.slice(0..100).should == []
      b.slice(-0..100).should == []
      b.slice(1..100).should == nil
      b.slice(-1..100).should == nil
    end

    it 'should throw an error on unknown argument types' do
      a = [1, 2, 3]
      -> { a.slice('a') }.should raise_error(TypeError, 'no implicit conversion of String into Integer')
      -> { a.slice('a', 'b') }.should raise_error(TypeError, 'no implicit conversion of String into Integer')
      -> { a.slice('a'..'c') }.should raise_error(TypeError, 'no implicit conversion of String into Integer')
      -> { a.slice(:foo) }.should raise_error(TypeError, 'no implicit conversion of Symbol into Integer')
      -> { a.slice(:foo, :bar) }.should raise_error(TypeError, 'no implicit conversion of Symbol into Integer')
      -> { a.slice(:foo..:bar) }.should raise_error(TypeError, 'no implicit conversion of Symbol into Integer')
      -> { a.slice([]) }.should raise_error(TypeError, 'no implicit conversion of Array into Integer')
      -> { a.slice([1]) }.should raise_error(TypeError, 'no implicit conversion of Array into Integer')
      -> { a.slice(nil) }.should raise_error(TypeError, 'no implicit conversion from nil to integer')
      -> { a.slice(1, nil) }.should raise_error(TypeError, 'no implicit conversion from nil to integer')
    end

    it 'raises a RangeError if passed a range with a bound that is too large' do
      array = [1, 2, 3, 4, 5, 6]
      -> { array.slice(bignum_value..(bignum_value + 1)) }.should raise_error(RangeError)
      -> { array.slice(0..bignum_value) }.should raise_error(RangeError)
    end
  end

  describe '#slice!' do
    context 'given one integer argument' do
      it 'should give nil for out of range elements' do
        a = []
        a.slice!(1).should == nil
        a.should == []

        a.slice!(-1).should == nil
        a.should == []

        a = [1]
        a.slice!(2).should == nil
        a.should == [1]

        a.slice!(-2).should == nil
        a.should == [1]
      end

      it 'should remove and return the element at the index' do
        a = [1, 2, 3]
        a.slice!(0).should == 1
        a.should == [2, 3]

        a.slice!(1).should == 3
        a.should == [2]
      end
    end

    context 'given two number arguments' do
      it 'should give nil for start value out of range' do
        a = []
        a.slice!(1, 10).should == nil
        a.slice!(1, 2).should == nil
        a.slice!(1, 3).should == nil
        a.slice!(1, 0).should == nil
        a.should == []

        a.slice!(-1, 1).should == nil
        a.slice!(-1, 0).should == nil
        a.slice!(-1, 2).should == nil
        a.should == []

        a = [1]
        a.slice!(2, 1).should == nil
        a.slice!(2, 2).should == nil
        a.slice!(2, 10).should == nil
        a.slice!(2, 0).should == nil
        a.should == [1]

        a.slice!(-2, 5).should == nil
        a.slice!(-2, 3).should == nil
        a.slice!(-2, 1).should == nil
        a.slice!(-2, 0).should == nil
        a.should == [1]
      end

      it 'should return nil for negative lengths' do
        a = [1, 2]

        a.slice!(0, -1).should == nil
        a.slice!(2, -1).should == nil
        a.slice!(1, -1).should == nil
        a.slice!(-1, -1).should == nil
        a.slice!(-2, -1).should == nil

        a.slice!(0, -1).should == nil
        a.slice!(2, -10).should == nil
        a.slice!(1, -12).should == nil
      end

      it 'should remove `length` elements from `start` always as array' do
        a = [1, 2, 3]
        a.slice!(1, 2).should == [2, 3]
        a.should == [1]

        a = [1, 2, 3]
        a.slice!(1, 1).should == [2]
        a.should == [1, 3]

        a = [1, 2, 3]
        a.slice!(0, 5).should == [1, 2, 3]
        a.should == []

        a = [1, 2, 3]
        a.slice!(2, 5).should == [3]
        a.should == [1, 2]

        a = [1, 2, 3]
        a.slice!(0, 0).should == []
        a.slice!(1, 0).should == []
        a.slice!(2, 0).should == []
        a.should == [1, 2, 3]
      end
    end

    context 'given a range' do
      it 'should return nil for start out of range' do
        a = []
        a.slice!(1..1).should == nil
        a.slice!(1...1).should == nil
        a.slice!(1..2).should == nil
        a.slice!(1..-1).should == nil
        a.slice!(-1..2).should == nil
        a.should == []

        a.slice!(-1..1).should == nil
        a.slice!(-1..0).should == nil
        a.slice!(-1..2).should == nil
        a.should == []

        a = [1]
        a.slice!(2..1).should == nil
        a.slice!(2..2).should == nil
        a.slice!(2..10).should == nil
        a.slice!(2..0).should == nil
        a.should == [1]

        a.slice!(-2..5).should == nil
        a.slice!(-2..3).should == nil
        a.slice!(-2..1).should == nil
        a.slice!(-2..0).should == nil
        a.should == [1]
      end

      it 'should remove and return the items at the indices from the range' do
        a = [1, 2, 3]
        a.slice!(1..2).should == [2, 3]
        a.should == [1]

        a = [1, 2, 3]
        a.slice!(1...2).should == [2]
        a.should == [1, 3]

        a = [1, 2, 3]
        a.slice!(1..1).should == [2]
        a.should == [1, 3]

        a = [1, 2, 3]
        a.slice!(1...1).should == []
        a.should == [1, 2, 3]

        a = [1, 2, 3]
        a.slice!(0..5).should == [1, 2, 3]
        a.should == []

        a = [1, 2, 3]
        a.slice!(2..5).should == [3]
        a.should == [1, 2]
      end
    end

    it 'should throw an error on unknown argument types' do
      a = [1, 2, 3]
      -> { a.slice!('a') }.should raise_error(TypeError, 'no implicit conversion of String into Integer')
      -> { a.slice!('a', 'b') }.should raise_error(TypeError, 'no implicit conversion of String into Integer')
      -> { a.slice!('a'..'c') }.should raise_error(TypeError, 'no implicit conversion of String into Integer')
      -> { a.slice!(:foo) }.should raise_error(TypeError, 'no implicit conversion of Symbol into Integer')
      -> { a.slice!(:foo, :bar) }.should raise_error(TypeError, 'no implicit conversion of Symbol into Integer')
      -> { a.slice!(:foo..:bar) }.should raise_error(TypeError, 'no implicit conversion of Symbol into Integer')
      -> { a.slice!([]) }.should raise_error(TypeError, 'no implicit conversion of Array into Integer')
      -> { a.slice!([1]) }.should raise_error(TypeError, 'no implicit conversion of Array into Integer')
      -> { a.slice!(nil) }.should raise_error(TypeError, 'no implicit conversion from nil to integer')
      -> { a.slice!(1, nil) }.should raise_error(TypeError, 'no implicit conversion from nil to integer')

      -> { a.slice!(1..2, nil) }.should raise_error(TypeError, 'no implicit conversion of Range into Integer')
      -> { a.slice!(1..2, []) }.should raise_error(TypeError, 'no implicit conversion of Range into Integer')
      -> { a.slice!(1..2, 'a') }.should raise_error(TypeError, 'no implicit conversion of Range into Integer')
      -> { a.slice!(1..2, :foo) }.should raise_error(TypeError, 'no implicit conversion of Range into Integer')
      -> { a.slice!(1..2, [[1, 2]]) }.should raise_error(TypeError, 'no implicit conversion of Range into Integer')
      -> { a.slice!(1..2, 3..4) }.should raise_error(TypeError, 'no implicit conversion of Range into Integer')
    end
  end

  describe '#hash' do
    it 'does not return the same hash for two different arrays' do
      [1, 1].hash.should_not == [2, 2].hash
      [[1, 2], [3, 2]].hash.should_not == [[1, 3], [2, 3]].hash
    end

    it 'does not collide when array contains duplicate elements' do
      found_hashes = []
      (0..100).each do |length|
        current_hash = (0..length).to_a.map { |x| 0 }.hash
        found_hashes.find_index(current_hash).should be_nil
        found_hashes << current_hash
      end
    end

    slow_test do
      it 'has a collision rate lower or equal to MRI' do
        # MRI Collision rate for this test suite is 0.11428571428571429%, rounded up to 0.115
        found_hashes = {}
        collisions = {}
        numeric_literals = (0..9).to_a
        string_literals = %w[r u b y MRI ruby]
        bool_literals = [true, false]
        floating_points = numeric_literals.map { |i| i / 2.2 }
        literals = numeric_literals + string_literals + bool_literals + floating_points
        (0..4).each do |length|
          literals.repeated_combination(length) do |combo|
            current_hash = combo.hash
            found_collision_combo = found_hashes[current_hash]
            if found_collision_combo
              next if found_collision_combo.eql? current_hash
              collisions[current_hash] = combo
            else
              found_hashes[current_hash] = combo
            end
          end
        end

        collisions_count = collisions.size
        total_count = found_hashes.size + collisions_count
        collision_rate = 1.0 * collisions_count / total_count
        if collision_rate > 0.115
          raise SpecFailedException,
                "collision rate is #{collision_rate} which is higher than MRI's collision rate of 0.115"
        end
      end
    end
  end
end
