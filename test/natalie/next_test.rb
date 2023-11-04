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

  it 'skips iteration in a while loop' do
    # wrapper because next was breaking out of the ^ above enclosing block
    wrapper = -> {
      a = 3
      while a.positive?
        a -= 1
        next if a == 1
      end
      a
    }

    wrapper.().should == 0
  end

  it 'skips iteration in an until loop' do
    # wrapper because next was breaking out of the ^ above enclosing block
    wrapper = -> {
      a = 3
      until a.negative?
        a -= 1
        next if a == 1
      end
      a
    }

    wrapper.().should == -1
  end
end
