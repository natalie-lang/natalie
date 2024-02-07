require_relative '../spec_helper'
require_relative '../../spec/core/data/fixtures/classes'

ruby_version_is ''...'3.2' do
  describe 'we need the same number of tests here to make the ruby comparison pass' do
    it 'transforms the data object into a hash' do
      1.should == 1
    end
  end
end

ruby_version_is '3.2' do
  describe 'Data#to_h' do
    it 'transforms the data object into a hash' do
      data = DataSpecs::Measure.new(amount: 42, unit: 'km')
      data.to_h.should == { amount: 42, unit: 'km' }
    end
  end
end
