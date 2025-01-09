require_relative '../spec_helper'
require_relative '../../spec/core/data/fixtures/classes'

describe 'Data#inspect' do
  it 'results in a readable representation' do
    data = DataSpecs::Measure.new(amount: 42, unit: 'km')
    data.inspect.should == '#<data DataSpecs::Measure amount=42, unit="km">'
  end
end

describe 'Data#to_s' do
  it 'results in a readable representation' do
    data = DataSpecs::Measure.new(amount: 42, unit: 'km')
    data.to_s.should == '#<data DataSpecs::Measure amount=42, unit="km">'
  end
end

describe 'Data#is_a?' do
  it 'should be a subclass of Data' do
    data = DataSpecs::Measure.new(amount: 42, unit: 'km')
    data.should be_kind_of(Data)
  end
end

describe 'Data#==' do
  it "returns true if the other is the same object" do
    measure = same_measure = DataSpecs::Measure.new(amount: 42, unit: 'km')
    measure.should == same_measure
  end

  it "returns true if the other has all the same fields" do
    measure = DataSpecs::Measure.new(amount: 42, unit: 'km')
    similar_measure = DataSpecs::Measure.new(amount: 42, unit: 'km')
    measure.should == similar_measure
  end

  it "returns false if the other is a different object or has different fields" do
    measure = DataSpecs::Measure.new(amount: 42, unit: 'km')
    different_measure = DataSpecs::Measure.new(amount: 26, unit: 'miles')
    measure.should != different_measure
  end

  it "returns false if other is of a different class" do
    measure = DataSpecs::Measure.new(amount: 42, unit: 'km')
    klass = Data.define(:amount, :unit)
    clone = klass.new(amount: 42, unit: 'km')
    measure.should != clone
  end
end
