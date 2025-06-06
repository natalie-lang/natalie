require_relative '../spec_helper'
require_relative 'fixtures/block'

describe "A block yielded a single" do
  before :all do
    def m(a) yield a end
  end

  context "Array" do
    it "assigns the Array to a single argument" do
      m([1, 2]) { |a| a }.should == [1, 2]
    end

    it "receives the identical Array object" do
      ary = [1, 2]
      m(ary) { |a| a }.should equal(ary)
    end

    it "assigns the Array to a single rest argument" do
      m([1, 2, 3]) { |*a| a }.should == [[1, 2, 3]]
    end

    it "assigns the first element to a single argument with trailing comma" do
      m([1, 2]) { |a, | a }.should == 1
    end

    it "assigns elements to required arguments" do
      m([1, 2, 3]) { |a, b| [a, b] }.should == [1, 2]
    end

    it "assigns nil to unassigned required arguments" do
      m([1, 2]) { |a, *b, c, d| [a, b, c, d] }.should == [1, [], 2, nil]
    end

    it "assigns elements to optional arguments" do
      m([1, 2]) { |a=5, b=4, c=3| [a, b, c] }.should == [1, 2, 3]
    end

    it "assigns elements to post arguments" do
      m([1, 2]) { |a=5, b, c, d| [a, b, c, d] }.should == [5, 1, 2, nil]
    end

    it "assigns elements to pre arguments" do
      m([1, 2]) { |a, b, c, d=5| [a, b, c, d] }.should == [1, 2, nil, 5]
    end

    it "assigns elements to pre and post arguments" do
      m([1               ]) { |a, b=5, c=6, d, e| [a, b, c, d, e] }.should == [1, 5, 6, nil, nil]
      m([1, 2            ]) { |a, b=5, c=6, d, e| [a, b, c, d, e] }.should == [1, 5, 6, 2, nil]
      m([1, 2, 3         ]) { |a, b=5, c=6, d, e| [a, b, c, d, e] }.should == [1, 5, 6, 2, 3]
      m([1, 2, 3, 4      ]) { |a, b=5, c=6, d, e| [a, b, c, d, e] }.should == [1, 2, 6, 3, 4]
      m([1, 2, 3, 4, 5   ]) { |a, b=5, c=6, d, e| [a, b, c, d, e] }.should == [1, 2, 3, 4, 5]
      m([1, 2, 3, 4, 5, 6]) { |a, b=5, c=6, d, e| [a, b, c, d, e] }.should == [1, 2, 3, 4, 5]
    end

    it "assigns elements to pre and post arguments when *rest is present" do
      m([1               ]) { |a, b=5, c=6, *d, e, f| [a, b, c, d, e, f] }.should == [1, 5, 6, [], nil, nil]
      m([1, 2            ]) { |a, b=5, c=6, *d, e, f| [a, b, c, d, e, f] }.should == [1, 5, 6, [], 2, nil]
      m([1, 2, 3         ]) { |a, b=5, c=6, *d, e, f| [a, b, c, d, e, f] }.should == [1, 5, 6, [], 2, 3]
      m([1, 2, 3, 4      ]) { |a, b=5, c=6, *d, e, f| [a, b, c, d, e, f] }.should == [1, 2, 6, [], 3, 4]
      m([1, 2, 3, 4, 5   ]) { |a, b=5, c=6, *d, e, f| [a, b, c, d, e, f] }.should == [1, 2, 3, [], 4, 5]
      m([1, 2, 3, 4, 5, 6]) { |a, b=5, c=6, *d, e, f| [a, b, c, d, e, f] }.should == [1, 2, 3, [4], 5, 6]
    end

    it "does not autosplat single argument to required arguments when a keyword rest argument is present" do
      m([1, 2]) { |a, **k| [a, k] }.should == [[1, 2], {}]
    end

    it "does not autosplat single argument to required arguments when keyword arguments are present" do
      m([1, 2]) { |a, b: :b, c: :c| [a, b, c] }.should == [[1, 2], :b, :c]
    end

    it "raises error when required keyword arguments are present" do
      -> {
        m([1, 2]) { |a, b:, c:| [a, b, c] }
      }.should raise_error(ArgumentError, "missing keywords: :b, :c")
    end

    it "assigns elements to mixed argument types" do
      result = m([1, 2, 3, {x: 9}]) { |a, b=5, *c, d, e: 2, **k| [a, b, c, d, e, k] }
      result.should == [1, 2, [3], {x: 9}, 2, {}]
    end

    it "does not treat final Hash as keyword arguments and does not autosplat" do
      result = m(["a" => 1, a: 10]) { |a=nil, **b| [a, b] }
      result.should == [[{"a" => 1, a: 10}], {}]
    end

    it "does not call #to_hash on final argument to get keyword arguments and does not autosplat" do
      suppress_keyword_warning do
        obj = mock("coerce block keyword arguments")
        obj.should_not_receive(:to_hash)

        result = m([obj]) { |a=nil, **b| [a, b] }
        result.should == [[obj], {}]
      end
    end

    it "does not call #to_hash on the argument when optional argument and keyword argument accepted and does not autosplat" do
      obj = mock("coerce block keyword arguments")
      obj.should_not_receive(:to_hash)

      result = m([obj]) { |a=nil, **b| [a, b] }
      result.should == [[obj], {}]
    end

    describe "when non-symbol keys are in a keyword arguments Hash" do
      it "does not separate non-symbol keys and symbol keys and does not autosplat" do
        suppress_keyword_warning do
          result = m(["a" => 10, b: 2]) { |a=nil, **b| [a, b] }
          result.should == [[{"a" => 10, b: 2}], {}]
        end
      end
    end

    it "does not treat hashes with string keys as keyword arguments and does not autosplat" do
      result = m(["a" => 10]) { |a = nil, **b| [a, b] }
      result.should == [[{"a" => 10}], {}]
    end

    it "does not call #to_hash on the last element if keyword arguments are present" do
      obj = mock("destructure block keyword arguments")
      obj.should_not_receive(:to_hash)

      result = m([1, 2, 3, obj]) { |a, *b, c, **k| [a, b, c, k] }
      result.should == [1, [2, 3], obj, {}]
    end

    it "does not call #to_hash on the last element when there are more arguments than parameters" do
      x = mock("destructure matching block keyword argument")
      x.should_not_receive(:to_hash)

      result = m([1, 2, 3, {y: 9}, 4, 5, x]) { |a, b=5, c, **k| [a, b, c, k] }
      result.should == [1, 2, 3, {}]
    end

    it "does not call #to_ary on the Array" do
      ary = [1, 2]
      ary.should_not_receive(:to_ary)

      m(ary) { |a, b, c| [a, b, c] }.should == [1, 2, nil]
    end
  end

  context "Object" do
    it "calls #to_ary on the object when taking multiple arguments" do
      obj = mock("destructure block arguments")
      obj.should_receive(:to_ary).and_return([1, 2])

      m(obj) { |a, b, c| [a, b, c] }.should == [1, 2, nil]
    end

    it "does not call #to_ary when not taking any arguments" do
      obj = mock("destructure block arguments")
      obj.should_not_receive(:to_ary)

      m(obj) { 1 }.should == 1
    end

    it "does not call #to_ary on the object when taking a single argument" do
      obj = mock("destructure block arguments")
      obj.should_not_receive(:to_ary)

      m(obj) { |a| a }.should == obj
    end

    it "does not call #to_ary on the object when taking a single rest argument" do
      obj = mock("destructure block arguments")
      obj.should_not_receive(:to_ary)

      m(obj) { |*a| a }.should == [obj]
    end

    it "receives the object if #to_ary returns nil" do
      obj = mock("destructure block arguments")
      obj.should_receive(:to_ary).and_return(nil)

      m(obj) { |a, b, c| [a, b, c] }.should == [obj, nil, nil]
    end

    it "receives the object if it does not respond to #to_ary" do
      obj = Object.new

      m(obj) { |a, b, c| [a, b, c] }.should == [obj, nil, nil]
    end

    it "calls #respond_to? to check if object has method #to_ary" do
      obj = mock("destructure block arguments")
      obj.should_receive(:respond_to?).with(:to_ary, true).and_return(true)
      obj.should_receive(:to_ary).and_return([1, 2])

      m(obj) { |a, b, c| [a, b, c] }.should == [1, 2, nil]
    end

    it "receives the object if it does not respond to #respond_to?" do
      obj = BasicObject.new

      m(obj) { |a, b, c| [a, b, c] }.should == [obj, nil, nil]
    end

    it "calls #to_ary on the object when it is defined dynamically" do
      obj = Object.new
      def obj.method_missing(name, *args, &block)
        if name == :to_ary
          [1, 2]
        else
          super
        end
      end
      def obj.respond_to_missing?(name, include_private)
        name == :to_ary
      end

      m(obj) { |a, b, c| [a, b, c] }.should == [1, 2, nil]
    end

    it "raises a TypeError if #to_ary does not return an Array" do
      obj = mock("destructure block arguments")
      obj.should_receive(:to_ary).and_return(1)

      -> { m(obj) { |a, b| } }.should raise_error(TypeError)
    end

    it "raises error transparently if #to_ary raises error on its own" do
      obj = Object.new
      def obj.to_ary; raise "Exception raised in #to_ary" end

      -> { m(obj) { |a, b| } }.should raise_error(RuntimeError, "Exception raised in #to_ary")
    end
  end
end

# TODO: rewrite
describe "A block" do
  before :each do
    @y = BlockSpecs::Yielder.new
  end

  it "captures locals from the surrounding scope" do
    var = 1
    @y.z { var }.should == 1
  end

  it "allows for a leading space before the arguments" do
    res = @y.s (:a){ 1 }
    res.should == 1
  end

  it "allows to define a block variable with the same name as the enclosing block" do
    o = BlockSpecs::OverwriteBlockVariable.new
    o.z { 1 }.should == 1
  end

  it "does not capture a local when an argument has the same name" do
    var = 1
    @y.s(2) { |var| var }.should == 2
    var.should == 1
  end

  it "does not capture a local when the block argument has the same name" do
    var = 1
    proc { |&var|
      var.call(2)
    }.call { |x| x }.should == 2
    var.should == 1
  end

  describe "taking zero arguments" do
    it "does not raise an exception when no values are yielded" do
      @y.z { 1 }.should == 1
    end

    it "does not raise an exception when values are yielded" do
      @y.s(0) { 1 }.should == 1
    end

    it "may include a rescue clause" do
      @y.z do raise ArgumentError; rescue ArgumentError; 7; end.should == 7
    end
  end

  describe "taking || arguments" do
    it "does not raise an exception when no values are yielded" do
      @y.z { || 1 }.should == 1
    end

    it "does not raise an exception when values are yielded" do
      @y.s(0) { || 1 }.should == 1
    end

    it "may include a rescue clause" do
      @y.z do || raise ArgumentError; rescue ArgumentError; 7; end.should == 7
    end
  end

  describe "taking |a| arguments" do
    it "assigns nil to the argument when no values are yielded" do
      @y.z { |a| a }.should be_nil
    end

    it "assigns the value yielded to the argument" do
      @y.s(1) { |a| a }.should == 1
    end

    it "does not call #to_ary to convert a single yielded object to an Array" do
      obj = mock("block yield to_ary")
      obj.should_not_receive(:to_ary)

      @y.s(obj) { |a| a }.should equal(obj)
    end

    it "assigns the first value yielded to the argument" do
      @y.m(1, 2) { |a| a }.should == 1
    end

    it "does not destructure a single Array value" do
      @y.s([1, 2]) { |a| a }.should == [1, 2]
    end

    it "may include a rescue clause" do
      @y.s(1) do |x| raise ArgumentError; rescue ArgumentError; 7; end.should == 7
    end
  end

  describe "taking |a, b| arguments" do
    it "assigns nil to the arguments when no values are yielded" do
      @y.z { |a, b| [a, b] }.should == [nil, nil]
    end

    it "assigns one value yielded to the first argument" do
      @y.s(1) { |a, b| [a, b] }.should == [1, nil]
    end

    it "assigns the first two values yielded to the arguments" do
      @y.m(1, 2, 3) { |a, b| [a, b] }.should == [1, 2]
    end

    it "does not destructure an Array value as one of several values yielded" do
      @y.m([1, 2], 3, 4) { |a, b| [a, b] }.should == [[1, 2], 3]
    end

    it "assigns 'nil' and 'nil' to the arguments when a single, empty Array is yielded" do
      @y.s([]) { |a, b| [a, b] }.should == [nil, nil]
    end

    it "assigns the element of a single element Array to the first argument" do
      @y.s([1]) { |a, b| [a, b] }.should == [1, nil]
      @y.s([nil]) { |a, b| [a, b] }.should == [nil, nil]
      @y.s([[]]) { |a, b| [a, b] }.should == [[], nil]
    end

    it "destructures a single Array value yielded" do
      @y.s([1, 2, 3]) { |a, b| [a, b] }.should == [1, 2]
    end

    it "destructures a splatted Array" do
      @y.r([[]]) { |a, b| [a, b] }.should == [nil, nil]
      @y.r([[1]]) { |a, b| [a, b] }.should == [1, nil]
    end

    it "calls #to_ary to convert a single yielded object to an Array" do
      obj = mock("block yield to_ary")
      obj.should_receive(:to_ary).and_return([1, 2])

      @y.s(obj) { |a, b| [a, b] }.should == [1, 2]
    end

    it "does not call #to_ary if the single yielded object is an Array" do
      obj = [1, 2]
      obj.should_not_receive(:to_ary)

      @y.s(obj) { |a, b| [a, b] }.should == [1, 2]
    end

    it "does not call #to_ary if the object does not respond to #to_ary" do
      obj = mock("block yield no to_ary")

      @y.s(obj) { |a, b| [a, b] }.should == [obj, nil]
    end

    it "raises a TypeError if #to_ary does not return an Array" do
      obj = mock("block yield to_ary invalid")
      obj.should_receive(:to_ary).and_return(1)

      -> { @y.s(obj) { |a, b| } }.should raise_error(TypeError)
    end

    it "raises the original exception if #to_ary raises an exception" do
      obj = mock("block yield to_ary raising an exception")
      obj.should_receive(:to_ary).and_raise(ZeroDivisionError)

      -> { @y.s(obj) { |a, b| } }.should raise_error(ZeroDivisionError)
    end
  end

  describe "taking |a, *b| arguments" do
    it "assigns 'nil' and '[]' to the arguments when no values are yielded" do
      @y.z { |a, *b| [a, b] }.should == [nil, []]
    end

    it "assigns all yielded values after the first to the rest argument" do
      @y.m(1, 2, 3) { |a, *b| [a, b] }.should == [1, [2, 3]]
    end

    it "assigns 'nil' and '[]' to the arguments when a single, empty Array is yielded" do
      @y.s([]) { |a, *b| [a, b] }.should == [nil, []]
    end

    it "assigns the element of a single element Array to the first argument" do
      @y.s([1]) { |a, *b| [a, b] }.should == [1, []]
      @y.s([nil]) { |a, *b| [a, b] }.should == [nil, []]
      @y.s([[]]) { |a, *b| [a, b] }.should == [[], []]
    end

    it "destructures a splatted Array" do
      @y.r([[]]) { |a, *b| [a, b] }.should == [nil, []]
      @y.r([[1]]) { |a, *b| [a, b] }.should == [1, []]
    end

    it "destructures a single Array value assigning the remaining values to the rest argument" do
      @y.s([1, 2, 3]) { |a, *b| [a, b] }.should == [1, [2, 3]]
    end

    it "calls #to_ary to convert a single yielded object to an Array" do
      obj = mock("block yield to_ary")
      obj.should_receive(:to_ary).and_return([1, 2])

      @y.s(obj) { |a, *b| [a, b] }.should == [1, [2]]
    end

    it "does not call #to_ary if the single yielded object is an Array" do
      obj = [1, 2]
      obj.should_not_receive(:to_ary)

      @y.s(obj) { |a, *b| [a, b] }.should == [1, [2]]
    end

    it "does not call #to_ary if the object does not respond to #to_ary" do
      obj = mock("block yield no to_ary")

      @y.s(obj) { |a, *b| [a, b] }.should == [obj, []]
    end

    it "raises a TypeError if #to_ary does not return an Array" do
      obj = mock("block yield to_ary invalid")
      obj.should_receive(:to_ary).and_return(1)

      -> { @y.s(obj) { |a, *b| } }.should raise_error(TypeError)
    end
  end

  describe "taking |*| arguments" do
    it "does not raise an exception when no values are yielded" do
      @y.z { |*| 1 }.should == 1
    end

    it "does not raise an exception when values are yielded" do
      @y.s(0) { |*| 1 }.should == 1
    end

    it "does not call #to_ary if the single yielded object is an Array" do
      obj = [1, 2]
      obj.should_not_receive(:to_ary)

      @y.s(obj) { |*| 1 }.should == 1
    end

    it "does not call #to_ary if the object does not respond to #to_ary" do
      obj = mock("block yield no to_ary")

      @y.s(obj) { |*| 1 }.should == 1
    end

    it "does not call #to_ary to convert a single yielded object to an Array" do
      obj = mock("block yield to_ary")
      obj.should_not_receive(:to_ary)

      @y.s(obj) { |*| 1 }.should == 1
    end
  end

  describe "taking |*a| arguments" do
    it "assigns '[]' to the argument when no values are yielded" do
      @y.z { |*a| a }.should == []
    end

    it "assigns a single value yielded to the argument as an Array" do
      @y.s(1) { |*a| a }.should == [1]
    end

    it "assigns all the values passed to the argument as an Array" do
      @y.m(1, 2, 3) { |*a| a }.should == [1, 2, 3]
    end

    it "assigns '[[]]' to the argument when passed an empty Array" do
      @y.s([]) { |*a| a }.should == [[]]
    end

    it "assigns a single Array value passed to the argument by wrapping it in an Array" do
      @y.s([1, 2, 3]) { |*a| a }.should == [[1, 2, 3]]
    end

    it "does not call #to_ary if the single yielded object is an Array" do
      obj = [1, 2]
      obj.should_not_receive(:to_ary)

      @y.s(obj) { |*a| a }.should == [[1, 2]]
    end

    it "does not call #to_ary if the object does not respond to #to_ary" do
      obj = mock("block yield no to_ary")

      @y.s(obj) { |*a| a }.should == [obj]
    end

    it "does not call #to_ary to convert a single yielded object to an Array" do
      obj = mock("block yield to_ary")
      obj.should_not_receive(:to_ary)

      @y.s(obj) { |*a| a }.should == [obj]
    end
  end

  describe "taking |a, | arguments" do
    it "assigns nil to the argument when no values are yielded" do
      @y.z { |a, | a }.should be_nil
    end

    it "assigns the argument a single value yielded" do
      @y.s(1) { |a, | a }.should == 1
    end

    it "assigns the argument the first value yielded" do
      @y.m(1, 2) { |a, | a }.should == 1
    end

    it "assigns the argument the first of several values yielded when it is an Array" do
      @y.m([1, 2], 3) { |a, | a }.should == [1, 2]
    end

    it "assigns nil to the argument when passed an empty Array" do
      @y.s([]) { |a, | a }.should be_nil
    end

    it "assigns the argument the first element of the Array when passed a single Array" do
      @y.s([1, 2]) { |a, | a }.should == 1
    end

    it "calls #to_ary to convert a single yielded object to an Array" do
      obj = mock("block yield to_ary")
      obj.should_receive(:to_ary).and_return([1, 2])

      @y.s(obj) { |a, | a }.should == 1
    end

    it "does not call #to_ary if the single yielded object is an Array" do
      obj = [1, 2]
      obj.should_not_receive(:to_ary)

      @y.s(obj) { |a, | a }.should == 1
    end

    it "does not call #to_ary if the object does not respond to #to_ary" do
      obj = mock("block yield no to_ary")

      @y.s(obj) { |a, | a }.should == obj
    end

    it "raises a TypeError if #to_ary does not return an Array" do
      obj = mock("block yield to_ary invalid")
      obj.should_receive(:to_ary).and_return(1)

      -> { @y.s(obj) { |a, | } }.should raise_error(TypeError)
    end
  end

  describe "taking |(a, b)| arguments" do
    it "assigns nil to the arguments when yielded no values" do
      @y.z { |(a, b)| [a, b] }.should == [nil, nil]
    end

    it "destructures a single Array value yielded" do
      @y.s([1, 2]) { |(a, b)| [a, b] }.should == [1, 2]
    end

    it "destructures a single Array value yielded when shadowing an outer variable" do
      a = 9
      @y.s([1, 2]) { |(a, b)| [a, b] }.should == [1, 2]
    end

    it "calls #to_ary to convert a single yielded object to an Array" do
      obj = mock("block yield to_ary")
      obj.should_receive(:to_ary).and_return([1, 2])

      @y.s(obj) { |(a, b)| [a, b] }.should == [1, 2]
    end

    it "does not call #to_ary if the single yielded object is an Array" do
      obj = [1, 2]
      obj.should_not_receive(:to_ary)

      @y.s(obj) { |(a, b)| [a, b] }.should == [1, 2]
    end

    it "does not call #to_ary if the object does not respond to #to_ary" do
      obj = mock("block yield no to_ary")

      @y.s(obj) { |(a, b)| [a, b] }.should == [obj, nil]
    end

    it "raises a TypeError if #to_ary does not return an Array" do
      obj = mock("block yield to_ary invalid")
      obj.should_receive(:to_ary).and_return(1)

      -> { @y.s(obj) { |(a, b)| } }.should raise_error(TypeError)
    end
  end

  describe "taking |(a, b), c| arguments" do
    it "assigns nil to the arguments when yielded no values" do
      @y.z { |(a, b), c| [a, b, c] }.should == [nil, nil, nil]
    end

    it "destructures a single one-level Array value yielded" do
      @y.s([1, 2]) { |(a, b), c| [a, b, c] }.should == [1, nil, 2]
    end

    it "destructures a single multi-level Array value yielded" do
      @y.s([[1, 2, 3], 4]) { |(a, b), c| [a, b, c] }.should == [1, 2, 4]
    end

    it "calls #to_ary to convert a single yielded object to an Array" do
      obj = mock("block yield to_ary")
      obj.should_receive(:to_ary).and_return([1, 2])

      @y.s(obj) { |(a, b), c| [a, b, c] }.should == [1, nil, 2]
    end

    it "does not call #to_ary if the single yielded object is an Array" do
      obj = [1, 2]
      obj.should_not_receive(:to_ary)

      @y.s(obj) { |(a, b), c| [a, b, c] }.should == [1, nil, 2]
    end

    it "does not call #to_ary if the object does not respond to #to_ary" do
      obj = mock("block yield no to_ary")

      @y.s(obj) { |(a, b), c| [a, b, c] }.should == [obj, nil, nil]
    end

    it "raises a TypeError if #to_ary does not return an Array" do
      obj = mock("block yield to_ary invalid")
      obj.should_receive(:to_ary).and_return(1)

      -> { @y.s(obj) { |(a, b), c| } }.should raise_error(TypeError)
    end
  end

  describe "taking nested |a, (b, (c, d))|" do
    it "assigns nil to the arguments when yielded no values" do
      @y.m { |a, (b, (c, d))| [a, b, c, d] }.should == [nil, nil, nil, nil]
    end

    it "destructures separate yielded values" do
      @y.m(1, 2) { |a, (b, (c, d))| [a, b, c, d] }.should == [1, 2, nil, nil]
    end

    it "destructures a nested Array value yielded" do
      @y.m(1, [2, 3]) { |a, (b, (c, d))| [a, b, c, d] }.should == [1, 2, 3, nil]
    end

    it "destructures a single multi-level Array value yielded" do
      @y.m(1, [2, [3, 4]]) { |a, (b, (c, d))| [a, b, c, d] }.should == [1, 2, 3, 4]
    end
  end

  describe "taking nested |a, ((b, c), d)|" do
    it "assigns nil to the arguments when yielded no values" do
      @y.m { |a, ((b, c), d)| [a, b, c, d] }.should == [nil, nil, nil, nil]
    end

    it "destructures separate yielded values" do
      @y.m(1, 2) { |a, ((b, c), d)| [a, b, c, d] }.should == [1, 2, nil, nil]
    end

    it "destructures a nested value yielded" do
      @y.m(1, [2, 3]) { |a, ((b, c), d)| [a, b, c, d] }.should == [1, 2, nil, 3]
    end

    it "destructures a single multi-level Array value yielded" do
      @y.m(1, [[2, 3], 4]) { |a, ((b, c), d)| [a, b, c, d] }.should == [1, 2, 3, 4]
    end
  end

  describe "taking |*a, b:|" do
    it "merges the hash into the splatted array" do
      @y.k { |*a, b:| [a, b] }.should == [[], true]
    end
  end

  describe "arguments with _" do
    it "extracts arguments with _" do
      @y.m([[1, 2, 3], 4]) { |(_, a, _), _| a }.should == 2
      @y.m([1, [2, 3, 4]]) { |_, (_, a, _)| a }.should == 3
    end

    it "assigns the first variable named" do
      @y.m(1, 2) { |_, _| _ }.should == 1
    end
  end

  describe "taking identically-named arguments" do
    it "raises a SyntaxError for standard arguments" do
      -> { eval "lambda { |x,x| }" }.should raise_error(SyntaxError)
      -> { eval "->(x,x) {}" }.should raise_error(SyntaxError)
      -> { eval "Proc.new { |x,x| }" }.should raise_error(SyntaxError)
    end

    it "accepts unnamed arguments" do
      lambda { |_,_| }.should be_an_instance_of(Proc) # rubocop:disable Style/Lambda
      -> _,_ {}.should be_an_instance_of(Proc)
      Proc.new { |_,_| }.should be_an_instance_of(Proc)
    end
  end

  describe 'pre and post parameters' do
    it "assigns nil to unassigned required arguments" do
      proc { |a, *b, c, d| [a, b, c, d] }.call(1, 2).should == [1, [], 2, nil]
    end

    it "assigns elements to optional arguments" do
      proc { |a=5, b=4, c=3| [a, b, c] }.call(1, 2).should == [1, 2, 3]
    end

    it "assigns elements to post arguments" do
      proc { |a=5, b, c, d| [a, b, c, d] }.call(1, 2).should == [5, 1, 2, nil]
    end

    it "assigns elements to pre arguments" do
      proc { |a, b, c, d=5| [a, b, c, d] }.call(1, 2).should == [1, 2, nil, 5]
    end

    it "assigns elements to pre and post arguments" do
      proc { |a, b=5, c=6, d, e| [a, b, c, d, e] }.call(1               ).should == [1, 5, 6, nil, nil]
      proc { |a, b=5, c=6, d, e| [a, b, c, d, e] }.call(1, 2            ).should == [1, 5, 6, 2, nil]
      proc { |a, b=5, c=6, d, e| [a, b, c, d, e] }.call(1, 2, 3         ).should == [1, 5, 6, 2, 3]
      proc { |a, b=5, c=6, d, e| [a, b, c, d, e] }.call(1, 2, 3, 4      ).should == [1, 2, 6, 3, 4]
      proc { |a, b=5, c=6, d, e| [a, b, c, d, e] }.call(1, 2, 3, 4, 5   ).should == [1, 2, 3, 4, 5]
      proc { |a, b=5, c=6, d, e| [a, b, c, d, e] }.call(1, 2, 3, 4, 5, 6).should == [1, 2, 3, 4, 5]
    end

    it "assigns elements to pre and post arguments when *rest is present" do
      proc { |a, b=5, c=6, *d, e, f| [a, b, c, d, e, f] }.call(1               ).should == [1, 5, 6, [], nil, nil]
      proc { |a, b=5, c=6, *d, e, f| [a, b, c, d, e, f] }.call(1, 2            ).should == [1, 5, 6, [], 2, nil]
      proc { |a, b=5, c=6, *d, e, f| [a, b, c, d, e, f] }.call(1, 2, 3         ).should == [1, 5, 6, [], 2, 3]
      proc { |a, b=5, c=6, *d, e, f| [a, b, c, d, e, f] }.call(1, 2, 3, 4      ).should == [1, 2, 6, [], 3, 4]
      proc { |a, b=5, c=6, *d, e, f| [a, b, c, d, e, f] }.call(1, 2, 3, 4, 5   ).should == [1, 2, 3, [], 4, 5]
      proc { |a, b=5, c=6, *d, e, f| [a, b, c, d, e, f] }.call(1, 2, 3, 4, 5, 6).should == [1, 2, 3, [4], 5, 6]
    end
  end
end

describe "Block-local variables" do
  it "are introduced with a semi-colon in the parameter list" do
    [1].map {|one; bl| bl }.should == [nil]
  end

  it "can be specified in a comma-separated list after the semi-colon" do
    [1].map {|one; bl, bl2| [bl, bl2] }.should == [[nil, nil]]
  end

  it "can not have the same name as one of the standard parameters" do
    -> { eval "[1].each {|foo; foo| }" }.should raise_error(SyntaxError)
    -> { eval "[1].each {|foo, bar; glark, bar| }" }.should raise_error(SyntaxError)
  end

  it "can not be prefixed with an asterisk" do
    -> { eval "[1].each {|foo; *bar| }" }.should raise_error(SyntaxError)
    -> do
      eval "[1].each {|foo, bar; glark, *fnord| }"
    end.should raise_error(SyntaxError)
  end

  it "can not be prefixed with an ampersand" do
    -> { eval "[1].each {|foo; &bar| }" }.should raise_error(SyntaxError)
    -> do
      eval "[1].each {|foo, bar; glark, &fnord| }"
    end.should raise_error(SyntaxError)
  end

  it "can not be assigned default values" do
    -> { eval "[1].each {|foo; bar=1| }" }.should raise_error(SyntaxError)
    -> do
      eval "[1].each {|foo, bar; glark, fnord=:fnord| }"
    end.should raise_error(SyntaxError)
  end

  it "need not be preceded by standard parameters" do
    [1].map {|; foo| foo }.should == [nil]
    [1].map {|; glark, bar| [glark, bar] }.should == [[nil, nil]]
  end

  it "only allow a single semi-colon in the parameter list" do
    -> { eval "[1].each {|foo; bar; glark| }" }.should raise_error(SyntaxError)
    -> { eval "[1].each {|; bar; glark| }" }.should raise_error(SyntaxError)
  end

  it "override shadowed variables from the outer scope" do
    out = :out
    [1].each {|; out| out = :in }
    NATFIXME 'it override shadowed variables from the outer scope', exception: SpecFailedException do
      out.should == :out
    end

    a = :a
    b = :b
    c = :c
    d = :d
    {ant: :bee}.each_pair do |a, b; c, d|
      a = :A
      b = :B
      c = :C
      d = :D
    end
    a.should == :a
    b.should == :b
    NATFIXME 'it override shadowed variables from the outer scope', exception: SpecFailedException do
      c.should == :c
      d.should == :d
    end
  end

  it "are not automatically instantiated in the outer scope" do
    defined?(glark).should be_nil
    [1].each {|;glark| 1}
    defined?(glark).should be_nil
  end

  it "are automatically instantiated in the block" do
    [1].each do |;glark|
      glark.should be_nil
    end
  end

  it "are visible in deeper scopes before initialization" do
    [1].each {|;glark|
      [1].each {
        defined?(glark).should_not be_nil
        glark = 1
      }
      glark.should == 1
    }
  end
end

describe "Post-args" do
  it "appear after a splat" do
    proc do |*a, b|
      [a, b]
    end.call(1, 2, 3).should == [[1, 2], 3]

    proc do |*a, b, c|
      [a, b, c]
    end.call(1, 2, 3).should == [[1], 2, 3]

    proc do |*a, b, c, d|
      [a, b, c, d]
    end.call(1, 2, 3).should == [[], 1, 2, 3]
  end

  it "are required for a lambda" do
    -> {
      -> *a, b do
        [a, b]
      end.call
    }.should raise_error(ArgumentError)
  end

  it "are assigned to nil when not enough arguments are given to a proc" do
    proc do |a, *b, c|
      [a, b, c]
    end.call.should == [nil, [], nil]
  end

  describe "with required args" do

    it "gathers remaining args in the splat" do
      proc do |a, *b, c|
        [a, b, c]
      end.call(1, 2, 3).should == [1, [2], 3]
    end

    it "has an empty splat when there are no remaining args" do
      proc do |a, b, *c, d|
        [a, b, c, d]
      end.call(1, 2, 3).should == [1, 2, [], 3]

      proc do |a, *b, c, d|
        [a, b, c, d]
      end.call(1, 2, 3).should == [1, [], 2, 3]
    end
  end

  describe "with optional args" do

    it "gathers remaining args in the splat" do
      proc do |a=5, *b, c|
        [a, b, c]
      end.call(1, 2, 3).should == [1, [2], 3]
    end

    it "overrides the optional arg before gathering in the splat" do
      proc do |a=5, *b, c|
        [a, b, c]
      end.call(2, 3).should == [2, [], 3]

      proc do |a=5, b=6, *c, d|
        [a, b, c, d]
      end.call(1, 2, 3).should == [1, 2, [], 3]

      proc do |a=5, *b, c, d|
        [a, b, c, d]
      end.call(1, 2, 3).should == [1, [], 2, 3]
    end

    it "uses the required arg before the optional and the splat" do
      proc do |a=5, *b, c|
        [a, b, c]
      end.call(3).should == [5, [], 3]

      proc do |a=5, b=6, *c, d|
        [a, b, c, d]
      end.call(3).should == [5, 6, [], 3]

      proc do |a=5, *b, c, d|
        [a, b, c, d]
      end.call(2, 3).should == [5, [], 2, 3]
    end

    it "overrides the optional args from left to right before gathering the splat" do
      proc do |a=5, b=6, *c, d|
        [a, b, c, d]
      end.call(2, 3).should == [2, 6, [], 3]
    end

    describe "with a circular argument reference" do
      ruby_version_is ""..."3.4" do
        it "raises a SyntaxError if using the argument in its default value" do
          a = 1
          -> {
            eval "proc { |a=a| a }"
          }.should raise_error(SyntaxError)
        end
      end

      ruby_version_is "3.4" do
        it "is nil if using the argument in its default value" do
          -> {
            eval "proc { |a=a| a }.call"
          }.call.should == nil
        end
      end
    end

    it "calls an existing method with the same name as the argument if explicitly using ()" do
      def a; 1; end
      proc { |a=a()| a }.call.should == 1
    end
  end

  describe "with pattern matching" do
    it "extracts matched blocks with post arguments" do
      proc do |(a, *b, c), d, e|
        [a, b, c, d, e]
      end.call([1, 2, 3, 4], 5, 6).should == [1, [2, 3], 4, 5, 6]
    end

    it "allows empty splats" do
      proc do |a, (*), b|
        [a, b]
      end.call([1, 2, 3]).should == [1, 3]
    end
  end
end

# tested more thoroughly in language/delegation_spec.rb
describe "Anonymous block forwarding" do
  it "forwards blocks to other method that formally declares anonymous block" do
    def b(&); c(&) end
    def c(&); yield :non_null end

    b { |c| c }.should == :non_null
  end

  it "requires the anonymous block parameter to be declared if directly passing a block" do
    -> { eval "def a; b(&); end; def b; end" }.should raise_error(SyntaxError)
  end

  it "works when it's the only declared parameter" do
    def inner; yield end
    def block_only(&); inner(&) end

    block_only { 1 }.should == 1
  end

  it "works alongside positional parameters" do
    def inner; yield end
    def pos(arg1, &); inner(&) end

    pos(:a) { 1 }.should == 1
  end

  it "works alongside positional arguments and splatted keyword arguments" do
    def inner; yield end
    def pos_kwrest(arg1, **kw, &); inner(&) end

    pos_kwrest(:a, arg: 3) { 1 }.should == 1
  end

  it "works alongside positional arguments and disallowed keyword arguments" do
    def inner; yield end
    def no_kw(arg1, **nil, &); inner(&) end

    no_kw(:a) { 1 }.should == 1
  end

  it "works alongside explicit keyword arguments" do
    eval <<-EOF
        def inner; yield end
        def rest_kw(*a, kwarg: 1, &); inner(&) end
        def kw(kwarg: 1, &); inner(&) end
        def pos_kw_kwrest(arg1, kwarg: 1, **kw, &); inner(&) end
        def pos_rkw(arg1, kwarg1:, &); inner(&) end
        def all(arg1, arg2, *rest, post1, post2, kw1: 1, kw2: 2, okw1:, okw2:, &); inner(&) end
        def all_kwrest(arg1, arg2, *rest, post1, post2, kw1: 1, kw2: 2, okw1:, okw2:, **kw, &); inner(&) end
    EOF

    rest_kw { 1 }.should == 1
    kw { 1 }.should == 1
    pos_kw_kwrest(:a) { 1 }.should == 1
    pos_rkw(:a, kwarg1: 3) { 1 }.should == 1
    all(:a, :b, :c, :d, :e, okw1: 'x', okw2: 'y') { 1 }.should == 1
    all_kwrest(:a, :b, :c, :d, :e, okw1: 'x', okw2: 'y') { 1 }.should == 1
  end
end

describe "`it` calls without arguments in a block with no ordinary parameters" do
  ruby_version_is "3.3"..."3.4" do
    it "emits a deprecation warning" do
      -> {
        eval "proc { it }"
      }.should complain(/warning: `it` calls without arguments will refer to the first block param in Ruby 3.4; use it\(\) or self.it/)
    end

    it "does not emit a deprecation warning when a block has parameters" do
      -> { eval "proc { |a, b| it }" }.should_not complain
      -> { eval "proc { |*rest| it }" }.should_not complain
      -> { eval "proc { |*| it }" }.should_not complain
      -> { eval "proc { |a:, b:| it }" }.should_not complain
      -> { eval "proc { |**kw| it }" }.should_not complain
      -> { eval "proc { |**| it }" }.should_not complain
      -> { eval "proc { |&block| it }" }.should_not complain
      -> { eval "proc { |&| it }" }.should_not complain
    end

    it "does not emit a deprecation warning when `it` calls with arguments" do
      -> { eval "proc { it(42) }" }.should_not complain
    end

    it "does not emit a deprecation warning when `it` calls with explicit empty arguments list" do
      -> { eval "proc { it() }" }.should_not complain
    end
  end
end

describe "if `it` is defined outside of a block" do
  it "treats `it` as a captured variable" do
    it = 5
    proc { it }.call(0).should == 5
  end
end
