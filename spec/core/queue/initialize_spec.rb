require_relative '../../spec_helper'

describe "Queue#initialize" do
  it "can be passed no arguments for an empty Queue" do
    q = Queue.new
    NATFIXME 'Implement Queue#size', exception: NoMethodError, message: "undefined method `size' for an instance of Queue" do
      q.size.should == 0
      q.should.empty?
    end
  end

  it "is a private method" do
    Queue.private_instance_methods.include?(:initialize).should == true
  end

  ruby_version_is '3.1' do
    it "adds all elements of the passed Enumerable to self" do
      NATFIXME 'Implement Queue#initialize', exception: ArgumentError, message: 'wrong number of arguments (given 1, expected 0)' do
        q = Queue.new([1, 2, 3])
        q.size.should == 3
        q.should_not.empty?
        q.pop.should == 1
        q.pop.should == 2
        q.pop.should == 3
        q.should.empty?
      end
    end

    describe "converts the given argument to an Array using #to_a" do
      it "uses #to_a on the provided Enumerable" do
        enumerable = MockObject.new('mock-enumerable')
        NATFIXME 'Implement Queue#initialize', exception: ArgumentError, message: 'wrong number of arguments (given 1, expected 0)' do
          enumerable.should_receive(:to_a).and_return([1, 2, 3])
          q = Queue.new(enumerable)
          q.size.should == 3
          q.should_not.empty?
          q.pop.should == 1
          q.pop.should == 2
          q.pop.should == 3
          q.should.empty?
        end
      end

      it "raises a TypeError if the given argument can't be converted to an Array" do
        NATFIXME 'Implement Queue#initialize', exception: SpecFailedException, message: /wrong number of arguments \(given 1, expected 0\)/ do
          -> { Queue.new(42) }.should raise_error(TypeError)
          -> { Queue.new(:abc) }.should raise_error(TypeError)
        end
      end

      it "raises a NoMethodError if the given argument raises a NoMethodError during type coercion to an Array" do
        enumerable = MockObject.new('mock-enumerable')
        NATFIXME 'Implement Queue#initialize', exception: SpecFailedException, message: /wrong number of arguments \(given 1, expected 0\)/ do
          enumerable.should_receive(:to_a).and_raise(NoMethodError)
          -> { Queue.new(enumerable) }.should raise_error(NoMethodError)
        end
      end
    end

    it "raises TypeError if the provided Enumerable does not respond to #to_a" do
      enumerable = MockObject.new('mock-enumerable')
      NATFIXME 'Implement Queue#initialize', exception: SpecFailedException, message: /wrong number of arguments \(given 1, expected 0\)/ do
        -> { Queue.new(enumerable) }.should raise_error(TypeError, "can't convert MockObject into Array")
      end
    end

    it "raises TypeError if #to_a does not return Array" do
      enumerable = MockObject.new('mock-enumerable')
      NATFIXME 'Implement Queue#initialize', exception: SpecFailedException, message: /wrong number of arguments \(given 1, expected 0\)/ do
        enumerable.should_receive(:to_a).and_return("string")

        -> { Queue.new(enumerable) }.should raise_error(TypeError, "can't convert MockObject to Array (MockObject#to_a gives String)")
      end
    end
  end
end
