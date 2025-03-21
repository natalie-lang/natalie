require_relative '../spec_helper'

# Ruby's mod (%) operator works differently than C when dealing with negative numbers.
describe 'modulo' do
  it 'works for bignums' do
    (100_000_000_000_000_000_000_000_000 % 9).should == 1
    (100_000_000_000_000_000_000_000_000 % -9).should == -8
    (100_000_000_000_000_000_000_000_000 % 100_000_000_000_000_000_000_000_000).should == 0
    (100_000_000_000_000_000_000_000_000 % -100_000_000_000_000_000_000_000_000).should == 0
  end

  it 'works for fixnums' do
    (1 % 10).should == 1
    (1 % -10).should == -9
    (-1 % 10).should == 9
    (-1 % -10).should == -1
    (1 % 1).should == 0
    (1 % -1).should == 0
    (-1 % 1).should == 0
    (-1 % -1).should == 0
  end

  it 'works for floats' do
    (1.0 % 10).should == 1.0
    (1.0 % -10).should == -9.0
    (-1.0 % 10).should == 9.0
    (-1.0 % -10).should == -1.0
    (1.0 % 1).should == 0.0
    (1.0 % -1).should == 0.0
    (-1.0 % 1).should == -0.0
    (-1.0 % -1).should == -0.0
  end
end
