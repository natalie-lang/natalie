require_relative '../spec_helper'

describe 'assignment order' do
  it 'can be defined and mutated in a single expression' do
    ((l = 1) + l).should == 2
    ((@i = 2) + @i).should == 4
    (($g = 3) + $g).should == 6
  end
end
