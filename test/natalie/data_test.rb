require_relative '../spec_helper'
require_relative '../../spec/core/data/fixtures/classes'

describe 'Data#is_a?' do
  it 'should be a subclass of Data' do
    data = DataSpecs::Measure.new(amount: 42, unit: 'km')
    data.should be_kind_of(Data)
  end
end

describe 'Data#==' do
  it 'returns true if the other is the same object' do
    measure = same_measure = DataSpecs::Measure.new(amount: 42, unit: 'km')
    measure.should == same_measure
  end

  it 'returns true if the other has all the same fields' do
    measure = DataSpecs::Measure.new(amount: 42, unit: 'km')
    similar_measure = DataSpecs::Measure.new(amount: 42, unit: 'km')
    measure.should == similar_measure
  end

  it 'returns false if the other is a different object or has different fields' do
    measure = DataSpecs::Measure.new(amount: 42, unit: 'km')
    different_measure = DataSpecs::Measure.new(amount: 26, unit: 'miles')
    measure.should != different_measure
  end

  it 'returns false if other is of a different class' do
    measure = DataSpecs::Measure.new(amount: 42, unit: 'km')
    klass = Data.define(:amount, :unit)
    clone = klass.new(amount: 42, unit: 'km')
    measure.should != clone
  end
end

describe 'Data#members' do
  it 'returns a new value every time' do
    members1 = DataSpecs::Measure.members
    members2 = DataSpecs::Measure.members
    members1.should_not equal(members2)

    data = DataSpecs::Measure.new(amount: 42, unit: 'km')
    members1 = data.members
    members2 = data.members
    members1.should_not equal(members2)
  end
end
