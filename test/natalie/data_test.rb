require_relative '../spec_helper'
require_relative '../../spec/core/data/fixtures/classes'

describe 'Data#is_a?' do
  it 'should be a subclass of Data' do
    data = DataSpecs::Measure.new(amount: 42, unit: 'km')
    data.should be_kind_of(Data)
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
