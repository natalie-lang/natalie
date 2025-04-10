require_relative '../../spec_helper'
require_relative 'fixtures/classes'

ruby_version_is "3.2" do
  describe "Data#==" do
    it "returns true if the other is the same object" do
      a = DataSpecs::Measure.new(42, "km")
      a.should == a
    end

    it "returns true if the other has all the same fields" do
      a = DataSpecs::Measure.new(42, "km")
      b = DataSpecs::Measure.new(42, "km")
      a.should == b
    end

    it "returns false if the other is a different object or has different fields" do
      a = DataSpecs::Measure.new(42, "km")
      b = DataSpecs::Measure.new(42, "mi")
      a.should_not == b
    end

    it "returns false if other is of a different class" do
      a = DataSpecs::Measure.new(42, "km")
      klass = Data.define(*DataSpecs::Measure.members)
      b = klass.new(42, "km")
      a.should_not == b
    end

    it "returns false if any corresponding elements are not equal with #==" do
      a = DataSpecs::Measure.new(42, "km")
      b = DataSpecs::Measure.new(42.0, "mi")
      a.should_not == b
    end

    # NATFIXME: Fix recursion
    xcontext "recursive structure" do
      it "returns true the other is the same object" do
        a = DataSpecs::Measure.allocate
        a.send(:initialize, amount: 42, unit: a)

        a.should == a
      end

      it "returns true if the other has all the same fields" do
        a = DataSpecs::Measure.allocate
        a.send(:initialize, amount: 42, unit: a)

        b = DataSpecs::Measure.allocate
        b.send(:initialize, amount: 42, unit: b)

        a.should == b
      end

      it "returns false if any corresponding elements are not equal with #==" do
        a = DataSpecs::Measure.allocate
        a.send(:initialize, amount: a, unit: "km")

        b = DataSpecs::Measure.allocate
        b.send(:initialize, amount: b, unit: "mi")

        a.should_not == b
      end
    end
  end
end
