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
      (['a', 'b', 'c'] <=> ['a', 'b', 'c']).should == 0
      (['a', 'b'] <=> ['a', 'b', 'c']).should == -1
      (['a', 'b', 'c'] <=> ['a', 'b']).should == 1
      (['a', 'b', 'C'] <=> ['a', 'b', 'c']).should == -1
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

  describe 'permutation' do
    it 'returns all non-repeating permutations of the array' do
      a = [1, 2, 3]
      a.permutation.to_a.should == [[1,2,3],[1,3,2],[2,1,3],[2,3,1],[3,1,2],[3,2,1]]
      a.permutation(1).to_a.should == [[1],[2],[3]]
      a.permutation(2).to_a.should == [[1,2],[1,3],[2,1],[2,3],[3,1],[3,2]]
      a.permutation(3).to_a.should == [[1,2,3],[1,3,2],[2,1,3],[2,3,1],[3,1,2],[3,2,1]]
      a.permutation(0).to_a.should == [[]] # one permutation of length 0
      a.permutation(4).to_a.should == []   # no permutations of length 4
      a = ('a'..'e').to_a
      a.permutation(2).to_a.should == [
        ["a", "b"],
        ["a", "c"],
        ["a", "d"],
        ["a", "e"],
        ["b", "a"],
        ["b", "c"],
        ["b", "d"],
        ["b", "e"],
        ["c", "a"],
        ["c", "b"],
        ["c", "d"],
        ["c", "e"],
        ["d", "a"],
        ["d", "b"],
        ["d", "c"],
        ["d", "e"],
        ["e", "a"],
        ["e", "b"],
        ["e", "c"],
        ["e", "d"]
      ]
      a.permutation(3).to_a.should == [
        ["a", "b", "c"],
        ["a", "b", "d"],
        ["a", "b", "e"],
        ["a", "c", "b"],
        ["a", "c", "d"],
        ["a", "c", "e"],
        ["a", "d", "b"],
        ["a", "d", "c"],
        ["a", "d", "e"],
        ["a", "e", "b"],
        ["a", "e", "c"],
        ["a", "e", "d"],
        ["b", "a", "c"],
        ["b", "a", "d"],
        ["b", "a", "e"],
        ["b", "c", "a"],
        ["b", "c", "d"],
        ["b", "c", "e"],
        ["b", "d", "a"],
        ["b", "d", "c"],
        ["b", "d", "e"],
        ["b", "e", "a"],
        ["b", "e", "c"],
        ["b", "e", "d"],
        ["c", "a", "b"],
        ["c", "a", "d"],
        ["c", "a", "e"],
        ["c", "b", "a"],
        ["c", "b", "d"],
        ["c", "b", "e"],
        ["c", "d", "a"],
        ["c", "d", "b"],
        ["c", "d", "e"],
        ["c", "e", "a"],
        ["c", "e", "b"],
        ["c", "e", "d"],
        ["d", "a", "b"],
        ["d", "a", "c"],
        ["d", "a", "e"],
        ["d", "b", "a"],
        ["d", "b", "c"],
        ["d", "b", "e"],
        ["d", "c", "a"],
        ["d", "c", "b"],
        ["d", "c", "e"],
        ["d", "e", "a"],
        ["d", "e", "b"],
        ["d", "e", "c"],
        ["e", "a", "b"],
        ["e", "a", "c"],
        ["e", "a", "d"],
        ["e", "b", "a"],
        ["e", "b", "c"],
        ["e", "b", "d"],
        ["e", "c", "a"],
        ["e", "c", "b"],
        ["e", "c", "d"],
        ["e", "d", "a"],
        ["e", "d", "b"],
        ["e", "d", "c"]
      ]
      a.permutation(5).to_a.should == [
        ["a", "b", "c", "d", "e"],
        ["a", "b", "c", "e", "d"],
        ["a", "b", "d", "c", "e"],
        ["a", "b", "d", "e", "c"],
        ["a", "b", "e", "c", "d"],
        ["a", "b", "e", "d", "c"],
        ["a", "c", "b", "d", "e"],
        ["a", "c", "b", "e", "d"],
        ["a", "c", "d", "b", "e"],
        ["a", "c", "d", "e", "b"],
        ["a", "c", "e", "b", "d"],
        ["a", "c", "e", "d", "b"],
        ["a", "d", "b", "c", "e"],
        ["a", "d", "b", "e", "c"],
        ["a", "d", "c", "b", "e"],
        ["a", "d", "c", "e", "b"],
        ["a", "d", "e", "b", "c"],
        ["a", "d", "e", "c", "b"],
        ["a", "e", "b", "c", "d"],
        ["a", "e", "b", "d", "c"],
        ["a", "e", "c", "b", "d"],
        ["a", "e", "c", "d", "b"],
        ["a", "e", "d", "b", "c"],
        ["a", "e", "d", "c", "b"],
        ["b", "a", "c", "d", "e"],
        ["b", "a", "c", "e", "d"],
        ["b", "a", "d", "c", "e"],
        ["b", "a", "d", "e", "c"],
        ["b", "a", "e", "c", "d"],
        ["b", "a", "e", "d", "c"],
        ["b", "c", "a", "d", "e"],
        ["b", "c", "a", "e", "d"],
        ["b", "c", "d", "a", "e"],
        ["b", "c", "d", "e", "a"],
        ["b", "c", "e", "a", "d"],
        ["b", "c", "e", "d", "a"],
        ["b", "d", "a", "c", "e"],
        ["b", "d", "a", "e", "c"],
        ["b", "d", "c", "a", "e"],
        ["b", "d", "c", "e", "a"],
        ["b", "d", "e", "a", "c"],
        ["b", "d", "e", "c", "a"],
        ["b", "e", "a", "c", "d"],
        ["b", "e", "a", "d", "c"],
        ["b", "e", "c", "a", "d"],
        ["b", "e", "c", "d", "a"],
        ["b", "e", "d", "a", "c"],
        ["b", "e", "d", "c", "a"],
        ["c", "a", "b", "d", "e"],
        ["c", "a", "b", "e", "d"],
        ["c", "a", "d", "b", "e"],
        ["c", "a", "d", "e", "b"],
        ["c", "a", "e", "b", "d"],
        ["c", "a", "e", "d", "b"],
        ["c", "b", "a", "d", "e"],
        ["c", "b", "a", "e", "d"],
        ["c", "b", "d", "a", "e"],
        ["c", "b", "d", "e", "a"],
        ["c", "b", "e", "a", "d"],
        ["c", "b", "e", "d", "a"],
        ["c", "d", "a", "b", "e"],
        ["c", "d", "a", "e", "b"],
        ["c", "d", "b", "a", "e"],
        ["c", "d", "b", "e", "a"],
        ["c", "d", "e", "a", "b"],
        ["c", "d", "e", "b", "a"],
        ["c", "e", "a", "b", "d"],
        ["c", "e", "a", "d", "b"],
        ["c", "e", "b", "a", "d"],
        ["c", "e", "b", "d", "a"],
        ["c", "e", "d", "a", "b"],
        ["c", "e", "d", "b", "a"],
        ["d", "a", "b", "c", "e"],
        ["d", "a", "b", "e", "c"],
        ["d", "a", "c", "b", "e"],
        ["d", "a", "c", "e", "b"],
        ["d", "a", "e", "b", "c"],
        ["d", "a", "e", "c", "b"],
        ["d", "b", "a", "c", "e"],
        ["d", "b", "a", "e", "c"],
        ["d", "b", "c", "a", "e"],
        ["d", "b", "c", "e", "a"],
        ["d", "b", "e", "a", "c"],
        ["d", "b", "e", "c", "a"],
        ["d", "c", "a", "b", "e"],
        ["d", "c", "a", "e", "b"],
        ["d", "c", "b", "a", "e"],
        ["d", "c", "b", "e", "a"],
        ["d", "c", "e", "a", "b"],
        ["d", "c", "e", "b", "a"],
        ["d", "e", "a", "b", "c"],
        ["d", "e", "a", "c", "b"],
        ["d", "e", "b", "a", "c"],
        ["d", "e", "b", "c", "a"],
        ["d", "e", "c", "a", "b"],
        ["d", "e", "c", "b", "a"],
        ["e", "a", "b", "c", "d"],
        ["e", "a", "b", "d", "c"],
        ["e", "a", "c", "b", "d"],
        ["e", "a", "c", "d", "b"],
        ["e", "a", "d", "b", "c"],
        ["e", "a", "d", "c", "b"],
        ["e", "b", "a", "c", "d"],
        ["e", "b", "a", "d", "c"],
        ["e", "b", "c", "a", "d"],
        ["e", "b", "c", "d", "a"],
        ["e", "b", "d", "a", "c"],
        ["e", "b", "d", "c", "a"],
        ["e", "c", "a", "b", "d"],
        ["e", "c", "a", "d", "b"],
        ["e", "c", "b", "a", "d"],
        ["e", "c", "b", "d", "a"],
        ["e", "c", "d", "a", "b"],
        ["e", "c", "d", "b", "a"],
        ["e", "d", "a", "b", "c"],
        ["e", "d", "a", "c", "b"],
        ["e", "d", "b", "a", "c"],
        ["e", "d", "b", "c", "a"],
        ["e", "d", "c", "a", "b"],
        ["e", "d", "c", "b", "a"]
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
      Array.new(5, :foo).should == [:foo, :foo, :foo, :foo, :foo]
    end
  end

  describe '#inspect' do
    it 'returns the syntax representation' do
      [1, 2, 3].inspect.should == "[1, 2, 3]"
    end
  end

  describe '#to_s' do
    it 'returns the syntax representation' do
      [1, 2, 3].to_s.should == "[1, 2, 3]"
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
  end

  describe '[]' do
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
  end

  describe '==' do
    it 'returns true if the two arrays are equal' do
      ([] == [1]).should == false
      ([] == []).should == true
      ([1, 2, 3] == [1, 2, 3]).should == true
      ([1, 2, 3] == [3, 2, 1]).should == false
    end
  end

  describe '#each' do
    it 'evaluates the block for each item in the array' do
      result = []
      [1, 2, 3].each do |i|
        result << i * 2
      end
      result.should == [2, 4, 6]
    end
  end

  describe '#each_with_index' do
    it 'evaluates the block for each item in the array, passing along an index too' do
      result = []
      ['a', 'b', 'c'].each_with_index do |c, i|
        result << [c, i]
      end
      result.should == [['a', 0], ['b', 1], ['c', 2]]
    end

    it 'returns an enumerator' do
      enum = ['a', 'b', 'c'].each_with_index
      enum.should be_an_instance_of(Enumerator)
      enum.to_a.should == [['a', 0], ['b', 1], ['c', 2]]
    end
  end

  describe '#map' do
    it 'returns a new array of the result of evaluating the block for each item in the array' do
      result = [1, 2, 3].map do |i|
        i * 2
      end
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
    it 'should return a new array of the last n elements' do
      [1, 2, 3].first(1).should == [1]
      [1, 2, 3].first(2).should == [1, 2]
      [1, 2, 3].first(3).should == [1, 2, 3]
      ['a', 'b'].first(1).should == ['a']
    end

   it 'returns an empty array if n == 0' do
     [1, 2, 3].first(0).should == []
     [].first(0).should == []
   end

    it 'returns empty array if the array is empty' do
      [].first(2).should == []
    end

    it 'Should raise ArgumentError on negative n' do
       r = begin
             [].first(-1)
             raise TestFailed 'Should raise'
           rescue ArgumentError => e
             e
           end
       r.message.should == 'negative array size'
    end
  end

  describe '#last(n)' do
    it 'should return a new array of the last n elements' do
      [1, 2, 3].last(1).should == [3]
      [1, 2, 3].last(2).should == [2, 3]
      [1, 2, 3].last(3).should == [1, 2, 3]
      ['a', 'b'].last(1).should == ['b']
    end

   it 'returns an empty array if n <= 0' do
     [1, 2, 3].last(0).should == []
     [].last(0).should == []
   end

    it 'returns empty array if the array is empty' do
      [].last(2).should == []
    end

    it 'Should raise ArgumentError on negative n' do
       r = begin
             [].last(-1)
             raise TestFailed 'Should raise'
           rescue ArgumentError => e
             e
           end
       r.message.should == 'negative array size'
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
      ['a', 'z', 'c', 'foo', 'bar'].sort.should == ['a', 'bar', 'c', 'foo', 'z']
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
      ['foo', 'bar'].join('').should == 'foobar'
      ['foo', 'bar'].join(',').should == 'foo,bar'
      [:foo, 2, nil, 'bar', {baz: :buz}].join(',').should == 'foo,2,,bar,{:baz=>:buz}'
    end

    it 'does not mutate anything' do
      a = ['foo', 'bar']
      a.join(' ')
      a.should == ['foo', 'bar']
    end
  end

  describe '#select' do
    it 'works' do
      [1, 2, 3, 4].select { |i| i % 2 == 0 }.should == [2, 4]
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

      a.each do |e|
        occurrences.include?(e).should == true
      end
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
      ['abc', 'mno', 'xyz'].max.should == 'xyz'
    end
  end

  describe '#min' do
    specify do
      [].min.should == nil
      [10].min.should == 10
      [1, 3].min.should == 1
      [1, 0, -1].min.should == -1
      ['abc', 'mno', 'xyz'].min.should == 'abc'
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

  describe '#index' do
    specify do
      a = [1, 2, 3]
      a.index(2).should == 1
      a = ['a', 'b', 'c']
      a.index('c').should == 2
      a.index('d').should == nil
      a.index(nil).should == nil
      a.index { |i| i == 'a' }.should == 0
      # TODO
      #a.index.should be_an_instance_of(Enumerator)
      #a.index.each { |i| i == 'c' }.should == 2
    end
  end

  describe '#uniq' do
    specify do
      [1, 1, 2, 1, 3, 3, 1.0].uniq.should == [1, 2, 3, 1.0]
      ['a', 'b', 'b', 'a'].uniq.should == ['a', 'b']
      o1 = Object.new
      o2 = Object.new
      [o1, o1, o2].uniq.should == [o1, o2]
    end
  end
end
