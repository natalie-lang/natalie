describe :array_slice, shared: true do
  it "returns the element at index with [index]" do
    [ "a", "b", "c", "d", "e" ].send(@method, 1).should == "b"

    a = [1, 2, 3, 4]

    a.send(@method, 0).should == 1
    a.send(@method, 1).should == 2
    a.send(@method, 2).should == 3
    a.send(@method, 3).should == 4
    a.send(@method, 4).should == nil
    a.send(@method, 10).should == nil

    a.should == [1, 2, 3, 4]
  end

  it "returns the element at index from the end of the array with [-index]" do
    [ "a", "b", "c", "d", "e" ].send(@method, -2).should == "d"

    a = [1, 2, 3, 4]

    a.send(@method, -1).should == 4
    a.send(@method, -2).should == 3
    a.send(@method, -3).should == 2
    a.send(@method, -4).should == 1
    a.send(@method, -5).should == nil
    a.send(@method, -10).should == nil

    a.should == [1, 2, 3, 4]
  end

  it "returns count elements starting from index with [index, count]" do
    [ "a", "b", "c", "d", "e" ].send(@method, 2, 3).should == ["c", "d", "e"]

    a = [1, 2, 3, 4]

    a.send(@method, 0, 0).should == []
    a.send(@method, 0, 1).should == [1]
    a.send(@method, 0, 2).should == [1, 2]
    a.send(@method, 0, 4).should == [1, 2, 3, 4]
    a.send(@method, 0, 6).should == [1, 2, 3, 4]
    a.send(@method, 0, -1).should == nil
    a.send(@method, 0, -2).should == nil
    a.send(@method, 0, -4).should == nil

    a.send(@method, 2, 0).should == []
    a.send(@method, 2, 1).should == [3]
    a.send(@method, 2, 2).should == [3, 4]
    a.send(@method, 2, 4).should == [3, 4]
    a.send(@method, 2, -1).should == nil

    a.send(@method, 4, 0).should == []
    a.send(@method, 4, 2).should == []
    a.send(@method, 4, -1).should == nil

    a.send(@method, 5, 0).should == nil
    a.send(@method, 5, 2).should == nil
    a.send(@method, 5, -1).should == nil

    a.send(@method, 6, 0).should == nil
    a.send(@method, 6, 2).should == nil
    a.send(@method, 6, -1).should == nil

    a.should == [1, 2, 3, 4]
  end

  it "returns count elements starting at index from the end of array with [-index, count]" do
    [ "a", "b", "c", "d", "e" ].send(@method, -2, 2).should == ["d", "e"]

    a = [1, 2, 3, 4]

    a.send(@method, -1, 0).should == []
    a.send(@method, -1, 1).should == [4]
    a.send(@method, -1, 2).should == [4]
    a.send(@method, -1, -1).should == nil

    a.send(@method, -2, 0).should == []
    a.send(@method, -2, 1).should == [3]
    a.send(@method, -2, 2).should == [3, 4]
    a.send(@method, -2, 4).should == [3, 4]
    a.send(@method, -2, -1).should == nil

    a.send(@method, -4, 0).should == []
    a.send(@method, -4, 1).should == [1]
    a.send(@method, -4, 2).should == [1, 2]
    a.send(@method, -4, 4).should == [1, 2, 3, 4]
    a.send(@method, -4, 6).should == [1, 2, 3, 4]
    a.send(@method, -4, -1).should == nil

    a.send(@method, -5, 0).should == nil
    a.send(@method, -5, 1).should == nil
    a.send(@method, -5, 10).should == nil
    a.send(@method, -5, -1).should == nil

    a.should == [1, 2, 3, 4]
  end

  it "returns the first count elements with [0, count]" do
    [ "a", "b", "c", "d", "e" ].send(@method, 0, 3).should == ["a", "b", "c"]
  end

  it "returns the subarray which is independent to self with [index,count]" do
    a = [1, 2, 3]
    sub = a.send(@method, 1,2)
    sub.replace([:a, :b])
    a.should == [1, 2, 3]
  end

  it "tries to convert the passed argument to an Integer using #to_int" do
    obj = mock('to_int')
    obj.stub!(:to_int).and_return(2)

    a = [1, 2, 3, 4]
    a.send(@method, obj).should == 3
    a.send(@method, obj, 1).should == [3]
    a.send(@method, obj, obj).should == [3, 4]
    a.send(@method, 0, obj).should == [1, 2]
  end

  it "raises TypeError if to_int returns non-integer" do
    from = mock('from')
    to = mock('to')

    # So we can construct a range out of them...
    def from.<=>(o) 0 end
    def to.<=>(o) 0 end

    a = [1, 2, 3, 4, 5]

    def from.to_int() 'cat' end
    def to.to_int() -2 end

    -> { a.send(@method, from..to) }.should raise_error(TypeError)

    def from.to_int() 1 end
    def to.to_int() 'cat' end

    -> { a.send(@method, from..to) }.should raise_error(TypeError)
  end

  it "returns the elements specified by Range indexes with [m..n]" do
    [ "a", "b", "c", "d", "e" ].send(@method, 1..3).should == ["b", "c", "d"]
    [ "a", "b", "c", "d", "e" ].send(@method, 4..-1).should == ['e']
    [ "a", "b", "c", "d", "e" ].send(@method, 3..3).should == ['d']
    [ "a", "b", "c", "d", "e" ].send(@method, 3..-2).should == ['d']
    ['a'].send(@method, 0..-1).should == ['a']

    a = [1, 2, 3, 4]

    a.send(@method, 0..-10).should == []
    a.send(@method, 0..0).should == [1]
    a.send(@method, 0..1).should == [1, 2]
    a.send(@method, 0..2).should == [1, 2, 3]
    a.send(@method, 0..3).should == [1, 2, 3, 4]
    a.send(@method, 0..4).should == [1, 2, 3, 4]
    a.send(@method, 0..10).should == [1, 2, 3, 4]

    a.send(@method, 2..-10).should == []
    a.send(@method, 2..0).should == []
    a.send(@method, 2..2).should == [3]
    a.send(@method, 2..3).should == [3, 4]
    a.send(@method, 2..4).should == [3, 4]

    a.send(@method, 3..0).should == []
    a.send(@method, 3..3).should == [4]
    a.send(@method, 3..4).should == [4]

    a.send(@method, 4..0).should == []
    a.send(@method, 4..4).should == []
    a.send(@method, 4..5).should == []

    a.send(@method, 5..0).should == nil
    a.send(@method, 5..5).should == nil
    a.send(@method, 5..6).should == nil

    a.should == [1, 2, 3, 4]
  end

  it "returns elements specified by Range indexes except the element at index n with [m...n]" do
    [ "a", "b", "c", "d", "e" ].send(@method, 1...3).should == ["b", "c"]

    a = [1, 2, 3, 4]

    a.send(@method, 0...-10).should == []
    a.send(@method, 0...0).should == []
    a.send(@method, 0...1).should == [1]
    a.send(@method, 0...2).should == [1, 2]
    a.send(@method, 0...3).should == [1, 2, 3]
    a.send(@method, 0...4).should == [1, 2, 3, 4]
    a.send(@method, 0...10).should == [1, 2, 3, 4]

    a.send(@method, 2...-10).should == []
    a.send(@method, 2...0).should == []
    a.send(@method, 2...2).should == []
    a.send(@method, 2...3).should == [3]
    a.send(@method, 2...4).should == [3, 4]

    a.send(@method, 3...0).should == []
    a.send(@method, 3...3).should == []
    a.send(@method, 3...4).should == [4]

    a.send(@method, 4...0).should == []
    a.send(@method, 4...4).should == []
    a.send(@method, 4...5).should == []

    a.send(@method, 5...0).should == nil
    a.send(@method, 5...5).should == nil
    a.send(@method, 5...6).should == nil

    a.should == [1, 2, 3, 4]
  end

  it "returns elements that exist if range start is in the array but range end is not with [m..n]" do
    [ "a", "b", "c", "d", "e" ].send(@method, 4..7).should == ["e"]
  end

  it "accepts Range instances having a negative m and both signs for n with [m..n] and [m...n]" do
    a = [1, 2, 3, 4]

    a.send(@method, -1..-1).should == [4]
    a.send(@method, -1...-1).should == []
    a.send(@method, -1..3).should == [4]
    a.send(@method, -1...3).should == []
    a.send(@method, -1..4).should == [4]
    a.send(@method, -1...4).should == [4]
    a.send(@method, -1..10).should == [4]
    a.send(@method, -1...10).should == [4]
    a.send(@method, -1..0).should == []
    a.send(@method, -1..-4).should == []
    a.send(@method, -1...-4).should == []
    a.send(@method, -1..-6).should == []
    a.send(@method, -1...-6).should == []

    a.send(@method, -2..-2).should == [3]
    a.send(@method, -2...-2).should == []
    a.send(@method, -2..-1).should == [3, 4]
    a.send(@method, -2...-1).should == [3]
    a.send(@method, -2..10).should == [3, 4]
    a.send(@method, -2...10).should == [3, 4]

    a.send(@method, -4..-4).should == [1]
    a.send(@method, -4..-2).should == [1, 2, 3]
    a.send(@method, -4...-2).should == [1, 2]
    a.send(@method, -4..-1).should == [1, 2, 3, 4]
    a.send(@method, -4...-1).should == [1, 2, 3]
    a.send(@method, -4..3).should == [1, 2, 3, 4]
    a.send(@method, -4...3).should == [1, 2, 3]
    a.send(@method, -4..4).should == [1, 2, 3, 4]
    a.send(@method, -4...4).should == [1, 2, 3, 4]
    a.send(@method, -4...4).should == [1, 2, 3, 4]
    a.send(@method, -4..0).should == [1]
    a.send(@method, -4...0).should == []
    a.send(@method, -4..1).should == [1, 2]
    a.send(@method, -4...1).should == [1]

    a.send(@method, -5..-5).should == nil
    a.send(@method, -5...-5).should == nil
    a.send(@method, -5..-4).should == nil
    a.send(@method, -5..-1).should == nil
    a.send(@method, -5..10).should == nil

    a.should == [1, 2, 3, 4]
  end

  it "returns the subarray which is independent to self with [m..n]" do
    a = [1, 2, 3]
    sub = a.send(@method, 1..2)
    sub.replace([:a, :b])
    a.should == [1, 2, 3]
  end

  it "tries to convert Range elements to Integers using #to_int with [m..n] and [m...n]" do
    from = mock('from')
    to = mock('to')

    # So we can construct a range out of them...
    def from.<=>(o) 0 end
    def to.<=>(o) 0 end

    def from.to_int() 1 end
    def to.to_int() -2 end

    a = [1, 2, 3, 4]

    a.send(@method, from..to).should == [2, 3]
    a.send(@method, from...to).should == [2]
    a.send(@method, 1..0).should == []
    a.send(@method, 1...0).should == []

    -> { a.send(@method, "a" .. "b") }.should raise_error(TypeError)
    -> { a.send(@method, "a" ... "b") }.should raise_error(TypeError)
    -> { a.send(@method, from .. "b") }.should raise_error(TypeError)
    -> { a.send(@method, from ... "b") }.should raise_error(TypeError)
  end

  it "returns the same elements as [m..n] and [m...n] with Range subclasses" do
    a = [1, 2, 3, 4]
    range_incl = ArraySpecs::MyRange.new(1, 2)
    range_excl = ArraySpecs::MyRange.new(-3, -1, true)

    a.send(@method, range_incl).should == [2, 3]
    a.send(@method, range_excl).should == [2, 3]
  end

  it "returns nil for a requested index not in the array with [index]" do
    [ "a", "b", "c", "d", "e" ].send(@method, 5).should == nil
  end

  it "returns [] if the index is valid but length is zero with [index, length]" do
    [ "a", "b", "c", "d", "e" ].send(@method, 0, 0).should == []
    [ "a", "b", "c", "d", "e" ].send(@method, 2, 0).should == []
  end

  it "returns nil if length is zero but index is invalid with [index, length]" do
    [ "a", "b", "c", "d", "e" ].send(@method, 100, 0).should == nil
    [ "a", "b", "c", "d", "e" ].send(@method, -50, 0).should == nil
  end

  # This is by design. It is in the official documentation.
  it "returns [] if index == array.size with [index, length]" do
    %w|a b c d e|.send(@method, 5, 2).should == []
  end

  it "returns nil if index > array.size with [index, length]" do
    %w|a b c d e|.send(@method, 6, 2).should == nil
  end

  it "returns nil if length is negative with [index, length]" do
    %w|a b c d e|.send(@method, 3, -1).should == nil
    %w|a b c d e|.send(@method, 2, -2).should == nil
    %w|a b c d e|.send(@method, 1, -100).should == nil
  end

  it "returns nil if no requested index is in the array with [m..n]" do
    [ "a", "b", "c", "d", "e" ].send(@method, 6..10).should == nil
  end

  it "returns nil if range start is not in the array with [m..n]" do
    [ "a", "b", "c", "d", "e" ].send(@method, -10..2).should == nil
    [ "a", "b", "c", "d", "e" ].send(@method, 10..12).should == nil
  end

  it "returns an empty array when m == n with [m...n]" do
    [1, 2, 3, 4, 5].send(@method, 1...1).should == []
  end

  it "returns an empty array with [0...0]" do
    [1, 2, 3, 4, 5].send(@method, 0...0).should == []
  end

  it "returns a subarray where m, n negatives and m < n with [m..n]" do
    [ "a", "b", "c", "d", "e" ].send(@method, -3..-2).should == ["c", "d"]
  end

  it "returns an array containing the first element with [0..0]" do
    [1, 2, 3, 4, 5].send(@method, 0..0).should == [1]
  end

  it "returns the entire array with [0..-1]" do
    [1, 2, 3, 4, 5].send(@method, 0..-1).should == [1, 2, 3, 4, 5]
  end

  it "returns all but the last element with [0...-1]" do
    [1, 2, 3, 4, 5].send(@method, 0...-1).should == [1, 2, 3, 4]
  end

  it "returns [3] for [2..-1] out of [1, 2, 3]" do
    [1,2,3].send(@method, 2..-1).should == [3]
  end

  it "returns an empty array when m > n and m, n are positive with [m..n]" do
    [1, 2, 3, 4, 5].send(@method, 3..2).should == []
  end

  it "returns an empty array when m > n and m, n are negative with [m..n]" do
    [1, 2, 3, 4, 5].send(@method, -2..-3).should == []
  end

  it "does not expand array when the indices are outside of the array bounds" do
    a = [1, 2]
    a.send(@method, 4).should == nil
    a.should == [1, 2]
    a.send(@method, 4, 0).should == nil
    a.should == [1, 2]
    a.send(@method, 6, 1).should == nil
    a.should == [1, 2]
    a.send(@method, 8...8).should == nil
    a.should == [1, 2]
    a.send(@method, 10..10).should == nil
    a.should == [1, 2]
  end

  describe "with a subclass of Array" do
    before :each do
      ScratchPad.clear

      @array = ArraySpecs::MyArray[1, 2, 3, 4, 5]
    end

    it "returns a Array instance with [n, m]" do
      @array.send(@method, 0, 2).should be_an_instance_of(Array)
    end

    it "returns a Array instance with [-n, m]" do
      @array.send(@method, -3, 2).should be_an_instance_of(Array)
    end

    it "returns a Array instance with [n..m]" do
      @array.send(@method, 1..3).should be_an_instance_of(Array)
    end

    it "returns a Array instance with [n...m]" do
      @array.send(@method, 1...3).should be_an_instance_of(Array)
    end

    it "returns a Array instance with [-n..-m]" do
      @array.send(@method, -3..-1).should be_an_instance_of(Array)
    end

    it "returns a Array instance with [-n...-m]" do
      @array.send(@method, -3...-1).should be_an_instance_of(Array)
    end

    it "returns an empty array when m == n with [m...n]" do
      @array.send(@method, 1...1).should == []
      ScratchPad.recorded.should be_nil
    end

    it "returns an empty array with [0...0]" do
      @array.send(@method, 0...0).should == []
      ScratchPad.recorded.should be_nil
    end

    it "returns an empty array when m > n and m, n are positive with [m..n]" do
      @array.send(@method, 3..2).should == []
      ScratchPad.recorded.should be_nil
    end

    it "returns an empty array when m > n and m, n are negative with [m..n]" do
      @array.send(@method, -2..-3).should == []
      ScratchPad.recorded.should be_nil
    end

    it "returns [] if index == array.size with [index, length]" do
      @array.send(@method, 5, 2).should == []
      ScratchPad.recorded.should be_nil
    end

    it "returns [] if the index is valid but length is zero with [index, length]" do
      @array.send(@method, 0, 0).should == []
      @array.send(@method, 2, 0).should == []
      ScratchPad.recorded.should be_nil
    end

    it "does not call #initialize on the subclass instance" do
      @array.send(@method, 0, 3).should == [1, 2, 3]
      ScratchPad.recorded.should be_nil
    end
  end

  it "raises a RangeError when the start index is out of range of Fixnum" do
    array = [1, 2, 3, 4, 5, 6]
    obj = mock('large value')
    obj.should_receive(:to_int).and_return(bignum_value)
    -> { array.send(@method, obj) }.should raise_error(RangeError)

    obj = 8e19
    -> { array.send(@method, obj) }.should raise_error(RangeError)

    # boundary value when longs are 64 bits
    -> { array.send(@method, 2.0**63) }.should raise_error(RangeError)

    # just under the boundary value when longs are 64 bits
    array.send(@method, max_long.to_f.prev_float).should == nil
  end

  it "raises a RangeError when the length is out of range of Fixnum" do
    array = [1, 2, 3, 4, 5, 6]
    obj = mock('large value')
    obj.should_receive(:to_int).and_return(bignum_value)
    -> { array.send(@method, 1, obj) }.should raise_error(RangeError)

    obj = 8e19
    -> { array.send(@method, 1, obj) }.should raise_error(RangeError)
  end

  it "raises a type error if a range is passed with a length" do
    ->{ [1, 2, 3].send(@method, 1..2, 1) }.should raise_error(TypeError)
  end

  it "raises a RangeError if passed a range with a bound that is too large" do
    array = [1, 2, 3, 4, 5, 6]
    -> { array.send(@method, bignum_value..(bignum_value + 1)) }.should raise_error(RangeError)
    -> { array.send(@method, 0..bignum_value) }.should raise_error(RangeError)
  end

  it "can accept endless ranges" do
    a = [0, 1, 2, 3, 4, 5]
    a.send(@method, eval("(2..)")).should == [2, 3, 4, 5]
    a.send(@method, eval("(2...)")).should == [2, 3, 4, 5]
    a.send(@method, eval("(-2..)")).should == [4, 5]
    a.send(@method, eval("(-2...)")).should == [4, 5]
    a.send(@method, eval("(9..)")).should == nil
    a.send(@method, eval("(9...)")).should == nil
    a.send(@method, eval("(-9..)")).should == nil
    a.send(@method, eval("(-9...)")).should == nil
  end

  describe "can be sliced with Enumerator::ArithmeticSequence" do
    before :each do
      @array = [0, 1, 2, 3, 4, 5]
    end

    it "has endless range and positive steps" do
      @array.send(@method, eval("(0..).step(1)")).should == [0, 1, 2, 3, 4, 5]
      @array.send(@method, eval("(0..).step(2)")).should == [0, 2, 4]
      @array.send(@method, eval("(0..).step(10)")).should == [0]

      @array.send(@method, eval("(2..).step(1)")).should == [2, 3, 4, 5]
      @array.send(@method, eval("(2..).step(2)")).should == [2, 4]
      @array.send(@method, eval("(2..).step(10)")).should == [2]

      @array.send(@method, eval("(-3..).step(1)")).should == [3, 4, 5]
      @array.send(@method, eval("(-3..).step(2)")).should == [3, 5]
      @array.send(@method, eval("(-3..).step(10)")).should == [3]
    end

    it "has beginless range and positive steps" do
      # end with zero index
      @array.send(@method, (..0).step(1)).should == [0]
      @array.send(@method, (...0).step(1)).should == []

      @array.send(@method, (..0).step(2)).should == [0]
      @array.send(@method, (...0).step(2)).should == []

      @array.send(@method, (..0).step(10)).should == [0]
      @array.send(@method, (...0).step(10)).should == []

      # end with positive index
      @array.send(@method, (..3).step(1)).should == [0, 1, 2, 3]
      @array.send(@method, (...3).step(1)).should == [0, 1, 2]

      @array.send(@method, (..3).step(2)).should == [0, 2]
      @array.send(@method, (...3).step(2)).should == [0, 2]

      @array.send(@method, (..3).step(10)).should == [0]
      @array.send(@method, (...3).step(10)).should == [0]

      # end with negative index
      @array.send(@method, (..-2).step(1)).should == [0, 1, 2, 3, 4,]
      @array.send(@method, (...-2).step(1)).should == [0, 1, 2, 3]

      @array.send(@method, (..-2).step(2)).should == [0, 2, 4]
      @array.send(@method, (...-2).step(2)).should == [0, 2]

      @array.send(@method, (..-2).step(10)).should == [0]
      @array.send(@method, (...-2).step(10)).should == [0]
    end

    it "has endless range and negative steps" do
      @array.send(@method, eval("(0..).step(-1)")).should == [0]
      @array.send(@method, eval("(0..).step(-2)")).should == [0]
      @array.send(@method, eval("(0..).step(-10)")).should == [0]

      @array.send(@method, eval("(2..).step(-1)")).should == [2, 1, 0]
      @array.send(@method, eval("(2..).step(-2)")).should == [2, 0]

      @array.send(@method, eval("(-3..).step(-1)")).should == [3, 2, 1, 0]
      @array.send(@method, eval("(-3..).step(-2)")).should == [3, 1]
    end

    it "has closed range and positive steps" do
      # start and end with 0
      @array.send(@method, eval("(0..0).step(1)")).should == [0]
      @array.send(@method, eval("(0...0).step(1)")).should == []

      @array.send(@method, eval("(0..0).step(2)")).should == [0]
      @array.send(@method, eval("(0...0).step(2)")).should == []

      @array.send(@method, eval("(0..0).step(10)")).should == [0]
      @array.send(@method, eval("(0...0).step(10)")).should == []

      # start and end with positive index
      @array.send(@method, eval("(1..3).step(1)")).should == [1, 2, 3]
      @array.send(@method, eval("(1...3).step(1)")).should == [1, 2]

      @array.send(@method, eval("(1..3).step(2)")).should == [1, 3]
      @array.send(@method, eval("(1...3).step(2)")).should == [1]

      @array.send(@method, eval("(1..3).step(10)")).should == [1]
      @array.send(@method, eval("(1...3).step(10)")).should == [1]

      # start with positive index, end with negative index
      @array.send(@method, eval("(1..-2).step(1)")).should == [1, 2, 3, 4]
      @array.send(@method, eval("(1...-2).step(1)")).should == [1, 2, 3]

      @array.send(@method, eval("(1..-2).step(2)")).should ==  [1, 3]
      @array.send(@method, eval("(1...-2).step(2)")).should ==  [1, 3]

      @array.send(@method, eval("(1..-2).step(10)")).should == [1]
      @array.send(@method, eval("(1...-2).step(10)")).should == [1]

      # start with negative index, end with positive index
      @array.send(@method, eval("(-4..4).step(1)")).should == [2, 3, 4]
      @array.send(@method, eval("(-4...4).step(1)")).should == [2, 3]

      @array.send(@method, eval("(-4..4).step(2)")).should == [2, 4]
      @array.send(@method, eval("(-4...4).step(2)")).should == [2]

      @array.send(@method, eval("(-4..4).step(10)")).should == [2]
      @array.send(@method, eval("(-4...4).step(10)")).should == [2]

      # start with negative index, end with negative index
      @array.send(@method, eval("(-4..-2).step(1)")).should == [2, 3, 4]
      @array.send(@method, eval("(-4...-2).step(1)")).should == [2, 3]

      @array.send(@method, eval("(-4..-2).step(2)")).should == [2, 4]
      @array.send(@method, eval("(-4...-2).step(2)")).should == [2]

      @array.send(@method, eval("(-4..-2).step(10)")).should == [2]
      @array.send(@method, eval("(-4...-2).step(10)")).should == [2]
    end

    it "has closed range and negative steps" do
      # start and end with 0
      @array.send(@method, eval("(0..0).step(-1)")).should == [0]
      @array.send(@method, eval("(0...0).step(-1)")).should == []

      @array.send(@method, eval("(0..0).step(-2)")).should == [0]
      @array.send(@method, eval("(0...0).step(-2)")).should == []

      @array.send(@method, eval("(0..0).step(-10)")).should == [0]
      @array.send(@method, eval("(0...0).step(-10)")).should == []

      # start and end with positive index
      @array.send(@method, eval("(1..3).step(-1)")).should == []
      @array.send(@method, eval("(1...3).step(-1)")).should == []

      @array.send(@method, eval("(1..3).step(-2)")).should == []
      @array.send(@method, eval("(1...3).step(-2)")).should == []

      @array.send(@method, eval("(1..3).step(-10)")).should == []
      @array.send(@method, eval("(1...3).step(-10)")).should == []

      # start with positive index, end with negative index
      @array.send(@method, eval("(1..-2).step(-1)")).should == []
      @array.send(@method, eval("(1...-2).step(-1)")).should == []

      @array.send(@method, eval("(1..-2).step(-2)")).should ==  []
      @array.send(@method, eval("(1...-2).step(-2)")).should ==  []

      @array.send(@method, eval("(1..-2).step(-10)")).should == []
      @array.send(@method, eval("(1...-2).step(-10)")).should == []

      # start with negative index, end with positive index
      @array.send(@method, eval("(-4..4).step(-1)")).should == []
      @array.send(@method, eval("(-4...4).step(-1)")).should == []

      @array.send(@method, eval("(-4..4).step(-2)")).should == []
      @array.send(@method, eval("(-4...4).step(-2)")).should == []

      @array.send(@method, eval("(-4..4).step(-10)")).should == []
      @array.send(@method, eval("(-4...4).step(-10)")).should == []

      # start with negative index, end with negative index
      @array.send(@method, eval("(-4..-2).step(-1)")).should == []
      @array.send(@method, eval("(-4...-2).step(-1)")).should == []

      @array.send(@method, eval("(-4..-2).step(-2)")).should == []
      @array.send(@method, eval("(-4...-2).step(-2)")).should == []

      @array.send(@method, eval("(-4..-2).step(-10)")).should == []
      @array.send(@method, eval("(-4...-2).step(-10)")).should == []
    end

    it "has inverted closed range and positive steps" do
      # start and end with positive index
      @array.send(@method, eval("(3..1).step(1)")).should == []
      @array.send(@method, eval("(3...1).step(1)")).should == []

      @array.send(@method, eval("(3..1).step(2)")).should == []
      @array.send(@method, eval("(3...1).step(2)")).should == []

      @array.send(@method, eval("(3..1).step(10)")).should == []
      @array.send(@method, eval("(3...1).step(10)")).should == []

      # start with negative index, end with positive index
      @array.send(@method, eval("(-2..1).step(1)")).should == []
      @array.send(@method, eval("(-2...1).step(1)")).should == []

      @array.send(@method, eval("(-2..1).step(2)")).should ==  []
      @array.send(@method, eval("(-2...1).step(2)")).should ==  []

      @array.send(@method, eval("(-2..1).step(10)")).should == []
      @array.send(@method, eval("(-2...1).step(10)")).should == []

      # start with positive index, end with negative index
      @array.send(@method, eval("(4..-4).step(1)")).should == []
      @array.send(@method, eval("(4...-4).step(1)")).should == []

      @array.send(@method, eval("(4..-4).step(2)")).should == []
      @array.send(@method, eval("(4...-4).step(2)")).should == []

      @array.send(@method, eval("(4..-4).step(10)")).should == []
      @array.send(@method, eval("(4...-4).step(10)")).should == []

      # start with negative index, end with negative index
      @array.send(@method, eval("(-2..-4).step(1)")).should == []
      @array.send(@method, eval("(-2...-4).step(1)")).should == []

      @array.send(@method, eval("(-2..-4).step(2)")).should == []
      @array.send(@method, eval("(-2...-4).step(2)")).should == []

      @array.send(@method, eval("(-2..-4).step(10)")).should == []
      @array.send(@method, eval("(-2...-4).step(10)")).should == []
    end

    it "has range with bounds outside of array" do
      # end is equal to array's length
      @array.send(@method, (0..6).step(1)).should == [0, 1, 2, 3, 4, 5]
      NATFIXME 'has range with bounds outside of array', exception: SpecFailedException do
        -> { @array.send(@method, (0..6).step(2)) }.should raise_error(RangeError)

        # end is greater than length with positive steps
        @array.send(@method, (1..6).step(2)).should == [1, 3, 5]
        @array.send(@method, (2..7).step(2)).should == [2, 4]
        -> { @array.send(@method, (2..8).step(2)) }.should raise_error(RangeError)

        # begin is greater than length with negative steps
        @array.send(@method, (6..1).step(-2)).should == [5, 3, 1]
        @array.send(@method, (7..2).step(-2)).should == [5, 3]
        -> { @array.send(@method, (8..2).step(-2)) }.should raise_error(RangeError)
      end
    end

    it "has endless range with start outside of array's bounds" do
      @array.send(@method, eval("(6..).step(1)")).should == []
      NATFIXME "has endless range with start outside of array's bounds", exception: SpecFailedException do
        @array.send(@method, eval("(7..).step(1)")).should == nil

        @array.send(@method, eval("(6..).step(2)")).should == []
        -> { @array.send(@method, eval("(7..).step(2)")) }.should raise_error(RangeError)
      end
    end
  end

  it "can accept beginless ranges" do
    a = [0, 1, 2, 3, 4, 5]
    a.send(@method, (..3)).should == [0, 1, 2, 3]
    a.send(@method, (...3)).should == [0, 1, 2]
    a.send(@method, (..-3)).should == [0, 1, 2, 3]
    a.send(@method, (...-3)).should == [0, 1, 2]
    a.send(@method, (..0)).should == [0]
    a.send(@method, (...0)).should == []
    a.send(@method, (..9)).should == [0, 1, 2, 3, 4, 5]
    a.send(@method, (...9)).should == [0, 1, 2, 3, 4, 5]
    a.send(@method, (..-9)).should == []
    a.send(@method, (...-9)).should == []
  end

  describe "can be sliced with Enumerator::ArithmeticSequence" do
    it "with infinite/inverted ranges and negative steps" do
      NATFIXME 'with infinite/inverted ranges and negative steps', exception: SpecFailedException do
        @array = [0, 1, 2, 3, 4, 5]
        @array.send(@method, (2..).step(-1)).should == [2, 1, 0]
        @array.send(@method, (2..).step(-2)).should == [2, 0]
        @array.send(@method, (2..).step(-3)).should == [2]
        @array.send(@method, (2..).step(-4)).should == [2]

        @array.send(@method, (-3..).step(-1)).should == [3, 2, 1, 0]
        @array.send(@method, (-3..).step(-2)).should == [3, 1]
        @array.send(@method, (-3..).step(-3)).should == [3, 0]
        @array.send(@method, (-3..).step(-4)).should == [3]
        @array.send(@method, (-3..).step(-5)).should == [3]

        @array.send(@method, (..0).step(-1)).should == [5, 4, 3, 2, 1, 0]
        @array.send(@method, (..0).step(-2)).should == [5, 3, 1]
        @array.send(@method, (..0).step(-3)).should == [5, 2]
        @array.send(@method, (..0).step(-4)).should == [5, 1]
        @array.send(@method, (..0).step(-5)).should == [5, 0]
        @array.send(@method, (..0).step(-6)).should == [5]
        @array.send(@method, (..0).step(-7)).should == [5]

        @array.send(@method, (...0).step(-1)).should == [5, 4, 3, 2, 1]
        @array.send(@method, (...0).step(-2)).should == [5, 3, 1]
        @array.send(@method, (...0).step(-3)).should == [5, 2]
        @array.send(@method, (...0).step(-4)).should == [5, 1]
        @array.send(@method, (...0).step(-5)).should == [5]
        @array.send(@method, (...0).step(-6)).should == [5]

        @array.send(@method, (...1).step(-1)).should == [5, 4, 3, 2]
        @array.send(@method, (...1).step(-2)).should == [5, 3]
        @array.send(@method, (...1).step(-3)).should == [5, 2]
        @array.send(@method, (...1).step(-4)).should == [5]
        @array.send(@method, (...1).step(-5)).should == [5]

        @array.send(@method, (..-5).step(-1)).should == [5, 4, 3, 2, 1]
        @array.send(@method, (..-5).step(-2)).should == [5, 3, 1]
        @array.send(@method, (..-5).step(-3)).should == [5, 2]
        @array.send(@method, (..-5).step(-4)).should == [5, 1]
        @array.send(@method, (..-5).step(-5)).should == [5]
        @array.send(@method, (..-5).step(-6)).should == [5]

        @array.send(@method, (...-5).step(-1)).should == [5, 4, 3, 2]
        @array.send(@method, (...-5).step(-2)).should == [5, 3]
        @array.send(@method, (...-5).step(-3)).should == [5, 2]
        @array.send(@method, (...-5).step(-4)).should == [5]
        @array.send(@method, (...-5).step(-5)).should == [5]

        @array.send(@method, (4..1).step(-1)).should == [4, 3, 2, 1]
        @array.send(@method, (4..1).step(-2)).should == [4, 2]
        @array.send(@method, (4..1).step(-3)).should == [4, 1]
        @array.send(@method, (4..1).step(-4)).should == [4]
        @array.send(@method, (4..1).step(-5)).should == [4]

        @array.send(@method, (4...1).step(-1)).should == [4, 3, 2]
        @array.send(@method, (4...1).step(-2)).should == [4, 2]
        @array.send(@method, (4...1).step(-3)).should == [4]
        @array.send(@method, (4...1).step(-4)).should == [4]

        @array.send(@method, (-2..1).step(-1)).should == [4, 3, 2, 1]
        @array.send(@method, (-2..1).step(-2)).should == [4, 2]
        @array.send(@method, (-2..1).step(-3)).should == [4, 1]
        @array.send(@method, (-2..1).step(-4)).should == [4]
        @array.send(@method, (-2..1).step(-5)).should == [4]

        @array.send(@method, (-2...1).step(-1)).should == [4, 3, 2]
        @array.send(@method, (-2...1).step(-2)).should == [4, 2]
        @array.send(@method, (-2...1).step(-3)).should == [4]
        @array.send(@method, (-2...1).step(-4)).should == [4]

        @array.send(@method, (4..-5).step(-1)).should == [4, 3, 2, 1]
        @array.send(@method, (4..-5).step(-2)).should == [4, 2]
        @array.send(@method, (4..-5).step(-3)).should == [4, 1]
        @array.send(@method, (4..-5).step(-4)).should == [4]
        @array.send(@method, (4..-5).step(-5)).should == [4]

        @array.send(@method, (4...-5).step(-1)).should == [4, 3, 2]
        @array.send(@method, (4...-5).step(-2)).should == [4, 2]
        @array.send(@method, (4...-5).step(-3)).should == [4]
        @array.send(@method, (4...-5).step(-4)).should == [4]

        @array.send(@method, (-2..-5).step(-1)).should == [4, 3, 2, 1]
        @array.send(@method, (-2..-5).step(-2)).should == [4, 2]
        @array.send(@method, (-2..-5).step(-3)).should == [4, 1]
        @array.send(@method, (-2..-5).step(-4)).should == [4]
        @array.send(@method, (-2..-5).step(-5)).should == [4]

        @array.send(@method, (-2...-5).step(-1)).should == [4, 3, 2]
        @array.send(@method, (-2...-5).step(-2)).should == [4, 2]
        @array.send(@method, (-2...-5).step(-3)).should == [4]
        @array.send(@method, (-2...-5).step(-4)).should == [4]
      end
    end
  end

  it "can accept nil...nil ranges" do
    a = [0, 1, 2, 3, 4, 5]
    a.send(@method, eval("(nil...nil)")).should == a
    a.send(@method, (...nil)).should == a
    a.send(@method, eval("(nil..)")).should == a
  end
end
