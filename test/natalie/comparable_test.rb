require_relative '../spec_helper'

describe 'comparable' do
  describe '#clamp' do
    it 'raises an ArgumentError for more than two arguments with the first argument is a range' do
      -> { 1.clamp(1..2, nil, nil) }.should raise_error(
                   ArgumentError,
                   'wrong number of arguments (given 3, expected 1..2)',
                 )
    end
  end
end
