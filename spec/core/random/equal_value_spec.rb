require_relative '../../spec_helper'

describe "Random#==" do
  it "returns true if the two objects have the same state" do
    a = Random.new(42)
    b = Random.new(42)
    NATFIXME 'Implement Random#state', exception: NoMethodError, message: /undefined method [`']state'/ do
      a.send(:state).should == b.send(:state)
    end
    NATFIXME 'Implement Random#==', exception: SpecFailedException do
      a.should == b
    end
  end

  it "returns false if the two objects have different state" do
    a = Random.new
    b = Random.new
    NATFIXME 'Implement Random#state', exception: NoMethodError, message: /undefined method [`']state'/ do
      a.send(:state).should_not == b.send(:state)
    end
    a.should_not == b
  end

  it "returns true if the two objects have the same seed" do
    a = Random.new(42)
    b = Random.new(42.5)
    a.seed.should == b.seed
    NATFIXME 'Implement Random#==', exception: SpecFailedException do
      a.should == b
    end
  end

  it "returns false if the two objects have a different seed" do
    a = Random.new(42)
    b = Random.new(41)
    a.seed.should_not == b.seed
    a.should_not == b
  end

  it "returns false if the other object is not a Random" do
    a = Random.new(42)
    a.should_not == 42
    a.should_not == [a]
  end
end
