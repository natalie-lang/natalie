require_relative '../spec_helper'

describe 'Kernel' do
  describe '#enum_for' do
    it 'returns a new Enumerator created by calling the given method' do
      a = [1, 2, 3]
      enum = a.enum_for(:each)
      enum.should be_an_instance_of(Enumerator)
      enum.to_a.should == [1, 2, 3]
    end
  end
end
