require_relative '../../spec_helper'

describe "Time#dup" do
  it "returns a Time object that represents the same time" do
    t = Time.at(100)
    t.dup.tv_sec.should == t.tv_sec
  end

  it "copies the gmt state flag" do
    NATFIXME 'Implement Time#gmtime', exception: NoMethodError, message: "undefined method 'gmtime' for an instance of Time" do
      Time.now.gmtime.dup.should.gmt?
    end
  end

  it "returns an independent Time object" do
    t = Time.now
    t2 = t.dup
    NATFIXME 'Implement Time#gmtime', exception: NoMethodError, message: "undefined method 'gmtime' for an instance of Time" do
      t.gmtime

      t2.should_not.gmt?
    end
  end

  it "returns a subclass instance" do
    c = Class.new(Time)
    t = c.now

    NATFIXME 'Time#now should preserve subclass', exception: SpecFailedException do
      t.should be_an_instance_of(c)
      t.dup.should be_an_instance_of(c)
    end
  end

  it "returns a clone of Time instance" do
    c = Time.dup
    t = c.now

    NATFIXME 'Time#now should preserve subclass', exception: SpecFailedException do
      t.should be_an_instance_of(c)
      t.should_not be_an_instance_of(Time)

      t.dup.should be_an_instance_of(c)
      t.dup.should_not be_an_instance_of(Time)
    end
  end

  it "does not copy frozen status from the original" do
    t = Time.now
    t.freeze
    t2 = t.dup
    t2.frozen?.should be_false
  end
end
