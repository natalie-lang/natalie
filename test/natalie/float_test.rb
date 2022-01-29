require_relative '../spec_helper'

describe 'float' do
  describe '#==' do
    it 'compares with bignums' do
      1124000727777607680000.0.should == 1124000727777607680000
    end
  end
end
