require_relative '../spec_helper'

describe 'while' do
  it 'works' do
    r = []
    x = 10
    while x != 0
      r << x
      x = x - 1
    end
    r.should == [10, 9, 8, 7, 6, 5, 4, 3, 2, 1]
  end
end

describe 'until' do
  it 'works' do
    r = []
    x = 10
    until x == 0
      r << x
      x = x - 1
    end
    r.should == [10, 9, 8, 7, 6, 5, 4, 3, 2, 1]
  end
end

describe 'each' do
  it 'can set variables outside the block' do
    x = 0
    [1, 2, 3].each do |i|
      x = i
    end
    x.should == 3
  end
end

describe 'loop' do
  it 'can be invoked' do
    s = false
    loop do
      s = true
      break
    end
    s.should == true
  end

  it 'loops' do
    x = 0
    loop do
      x += 1
      break if x > 3
    end
    x.should == 4
  end

  it 'can handle break expr' do
    x = 0
    s = loop do
      x += 1
      break 'hello' if x > 3
    end
    s.should == 'hello'
    x.should == 4
  end
end
