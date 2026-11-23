require_relative '../../spec_helper'

describe "Method#to_proc" do
  it "preserves a captured environment, receiver, and parameter metadata" do
    klass = Class.new do
      captured = 7
      define_method(:value) { |extra| captured + extra }
    end
    method = klass.new.method(:value)
    callable = method.to_proc
    GC.start
    callable.call(2).should == 9
    callable.parameters.should == method.parameters
    callable.source_location.should == method.source_location
    callable.lambda?.should be_true
    -> { callable.call }.should raise_error(ArgumentError)
  end

  it "preserves super when called through a Proc" do
    parent = Class.new { def value; 7; end }
    child = Class.new(parent) { def value; super + 2; end }
    child.new.method(:value).to_proc.call.should == 9
  end

  it "uses method argument rules for a body originally compiled as a block" do
    klass = Class.new do
      define_method(:pair) { |a, b = 2| [a, b] }
    end
    method = klass.new.method(:pair)
    method.to_proc.call(1).should == [1, 2]
    method.to_proc.call([1, 2]).should == [[1, 2], 2]
    -> { method.call }.should raise_error(ArgumentError)
    -> { method.call(1, 2, 3) }.should raise_error(ArgumentError)
    -> { method.to_proc.call(1, 2, 3) }.should raise_error(ArgumentError)
    original = proc { |a, b| [a, b] }
    original.call([1, 2]).should == [1, 2]
    original.call.should == [nil, nil]
  end

  it "keeps its bound receiver when installed as another method" do
    source = Object.new
    def source.value; self; end
    target = Class.new
    target.define_method(:value, &source.method(:value))
    target.new.value.should equal(source)
  end
end
