require_relative '../../spec_helper'
require_relative 'fixtures/classes'
require_relative 'shared/attr_added'

describe "Module#attr_accessor" do
  it "creates a getter and setter for each given attribute name" do
    c = Class.new do
      attr_accessor :a, "b"
    end

    o = c.new

    ['a','b'].each do |x|
      o.respond_to?(x).should == true
      o.respond_to?("#{x}=").should == true
    end

    o.a = "a"
    o.a.should == "a"

    o.b = "b"
    o.b.should == "b"
    o.a = o.b = nil

    o.send(:a=,"a")
    o.send(:a).should == "a"

    o.send(:b=, "b")
    o.send(:b).should == "b"
  end

  it "not allows creating an attr_accessor on an immediate class" do
    class TrueClass
      attr_accessor :spec_attr_accessor
    end

    -> { true.spec_attr_accessor = "a" }.should raise_error(FrozenError)
  end

  it "raises FrozenError if the receiver if frozen" do
    c = Class.new do
      attr_accessor :foo
    end
    obj = c.new
    obj.foo = 1
    obj.foo.should == 1

    obj.freeze
    -> { obj.foo = 42 }.should raise_error(FrozenError)
    obj.foo.should == 1
  end

  it "converts non string/symbol names to strings using to_str" do
    (o = mock('test')).should_receive(:to_str).any_number_of_times.and_return("test")
    c = Class.new do
      attr_accessor o
    end

    c.new.respond_to?("test").should == true
    c.new.respond_to?("test=").should == true
  end

  it "raises a TypeError when the given names can't be converted to strings using to_str" do
    o = mock('o')
    -> { Class.new { attr_accessor o } }.should raise_error(TypeError)
    (o = mock('123')).should_receive(:to_str).and_return(123)
    -> { Class.new { attr_accessor o } }.should raise_error(TypeError)
  end

  it "applies current visibility to methods created" do
    c = Class.new do
      protected
      attr_accessor :foo
    end

    -> { c.new.foo }.should raise_error(NoMethodError)
    -> { c.new.foo=1 }.should raise_error(NoMethodError)
  end

  it "is a public method" do
    Module.should have_public_instance_method(:attr_accessor, false)
  end

  it "returns an array of defined method names as symbols" do
    Class.new do
      (attr_accessor :foo, 'bar').should == [:foo, :foo=, :bar, :bar=]
    end
  end

  describe "on immediates" do
    before :each do
      class Integer
        attr_accessor :foobar
      end
    end

    after :each do
      if Integer.method_defined?(:foobar)
        Integer.send(:remove_method, :foobar)
      end
      if Integer.method_defined?(:foobar=)
        Integer.send(:remove_method, :foobar=)
      end
    end

    it "can read through the accessor" do
      1.foobar.should be_nil
    end
  end

  it_behaves_like :module_attr_added, :attr_accessor
end
