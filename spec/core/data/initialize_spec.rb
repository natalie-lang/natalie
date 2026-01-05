require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe "Data#initialize" do
  it "accepts positional arguments" do
    data = DataSpecs::Measure.new(42, "km")

    data.amount.should == 42
    data.unit.should == "km"
  end

  it "accepts alternative positional arguments" do
    data = DataSpecs::Measure[42, "km"]

    data.amount.should == 42
    data.unit.should == "km"
  end

  it "accepts keyword arguments" do
    data = DataSpecs::Measure.new(amount: 42, unit: "km")

    data.amount.should == 42
    data.unit.should == "km"
  end

  it "accepts alternative keyword arguments" do
    data = DataSpecs::Measure[amount: 42, unit: "km"]

    data.amount.should == 42
    data.unit.should == "km"
  end

  it "accepts String keyword arguments" do
    data = DataSpecs::Measure.new("amount" => 42, "unit" => "km")

    data.amount.should == 42
    data.unit.should == "km"
  end

  it "raises ArgumentError if no arguments are given" do
    -> {
      DataSpecs::Measure.new
    }.should raise_error(ArgumentError) { |e|
      e.message.should.include?("missing keywords: :amount, :unit")
    }
  end

  it "raises ArgumentError if at least one argument is missing" do
    -> {
      DataSpecs::Measure.new(unit: "km")
    }.should raise_error(ArgumentError) { |e|
      e.message.should.include?("missing keyword: :amount")
    }
  end

  it "raises ArgumentError if unknown keyword is given" do
    -> {
      DataSpecs::Measure.new(amount: 42, unit: "km", system: "metric")
    }.should raise_error(ArgumentError) { |e|
      e.message.should.include?("unknown keyword: :system")
    }
  end

  it "supports super from a subclass" do
    ms = DataSpecs::MeasureSubclass.new(amount: 1, unit: "km")

    ms.amount.should == 1
    ms.unit.should == "km"
  end

  it "supports Data with no fields" do
    NATFIXME 'it supports Data with no fields', exception: SpecFailedException do
      -> { DataSpecs::Empty.new }.should_not raise_error
    end
  end

  it "can be overridden" do
    ScratchPad.record []

    measure_class = Data.define(:amount, :unit) do
      def initialize(*, **)
        super
        ScratchPad << :initialize
      end
    end

    NATFIXME 'it can be overridden', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 0)' do
      measure_class.new(42, "m")
      ScratchPad.recorded.should == [:initialize]
    end
  end

  context "when it is overridden" do
    it "is called with keyword arguments when given positional arguments" do
      ScratchPad.clear
      NATFIXME 'it is called with keyword arguments when given positional arguments', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 0)' do
        DataSpecs::DataWithOverriddenInitialize.new(42, "m")
        ScratchPad.recorded.should == [:initialize, [], {amount: 42, unit: "m"}]
      end
    end

    it "is called with keyword arguments when given keyword arguments" do
      ScratchPad.clear
      NATFIXME 'it is called with keyword arguments when given keyword arguments', exception: ArgumentError, message: 'wrong number of arguments (given 1, expected 0)' do
        DataSpecs::DataWithOverriddenInitialize.new(amount: 42, unit: "m")
        ScratchPad.recorded.should == [:initialize, [], {amount: 42, unit: "m"}]
      end
    end

    it "is called with keyword arguments when given alternative positional arguments" do
      ScratchPad.clear
      NATFIXME 'it is called with keyword arguments when given alternative positional arguments', exception: ArgumentError, message: 'wrong number of arguments (given 3, expected 0)' do
        DataSpecs::DataWithOverriddenInitialize[42, "m"]
        ScratchPad.recorded.should == [:initialize, [], {amount: 42, unit: "m"}]
      end
    end

    it "is called with keyword arguments when given alternative keyword arguments" do
      ScratchPad.clear
      NATFIXME 'it is called with keyword arguments when given alternative keyword arguments', exception: ArgumentError, message: 'wrong number of arguments (given 1, expected 0)' do
        DataSpecs::DataWithOverriddenInitialize[amount: 42, unit: "m"]
        ScratchPad.recorded.should == [:initialize, [], {amount: 42, unit: "m"}]
      end
    end

    # See https://github.com/ruby/psych/pull/765
    it "can be deserialized by calling Data.instance_method(:initialize)" do
      NATFIXME 'it can be deserialized by calling Data.instance_method(:initialize)', exception: ArgumentError, message: 'wrong number of arguments (given 1, expected 0)' do
        d1 = DataSpecs::Area.new(width: 2, height: 3)
        d1.area.should == 6

        d2 = DataSpecs::Area.allocate
        Data.instance_method(:initialize).bind_call(d2, **d1.to_h)
        d2.should == d1
      end
    end
  end
end
