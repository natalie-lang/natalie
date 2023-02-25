require_relative '../spec_helper'

describe 'call order test' do
  @order = []

  def first
    @order << 1
    1
  end

  def second
    @order << 2
    2
  end

  it 'evaluates the receiver before the arguments' do
    (first + second).should == 3
    @order.should == [1, 2]
  end

  describe 'assignment' do
    it 'can be assigned and mutated in a single expression' do
      ((l = 1) + l).should == 2
      ((@i = 2) + @i).should == 4
      (($g = 3) + $g).should == 6
    end
  end
end
