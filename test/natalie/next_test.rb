require_relative '../spec_helper'

describe 'next' do
  it 'skips an iteration in a block' do
    result =
      [1, 2, 3].map do |i|
        next if i < 3
        i
      end
    result.should == [nil, nil, 3]
    result =
      [1, 2, 3].map do |i|
        next 0 if i < 3
        i
      end
    result.should == [0, 0, 3]
  end
end
