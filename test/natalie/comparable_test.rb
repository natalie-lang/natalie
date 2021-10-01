require_relative '../spec_helper'

describe 'comparable' do
  describe '#clamp' do
    it 'raises an Argument error if the range parameter is exclusive' do
      -> { 1.clamp(1...2) }.should raise_error(ArgumentError)
    end
  end
end
