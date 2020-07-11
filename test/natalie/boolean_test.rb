require_relative '../spec_helper'

describe 'booleans' do
  it 'cannot be created with .new' do
    -> { TrueClass.new }.should raise_error(NoMethodError)
    -> { FalseClass.new }.should raise_error(NoMethodError)
  end

  describe 'equality' do
    it 'is equal to other booleans' do
      r = true == true
      r.should == true
      r = false == false
      r.should == true
      r = true == false
      r.should == false
      r = false == true
      r.should == false
    end
  end

  describe 'inspect' do
    it 'returns its name' do
      true.inspect.should == 'true'
      false.inspect.should == 'false'
    end
  end
end
