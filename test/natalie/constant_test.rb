require_relative '../spec_helper'

FOO = 1
Bar = 2

describe 'constants' do
  it 'works' do
    FOO.should == 1
    Bar.should == 2
    -> { Baz }.should raise_error(NameError, /uninitialized constant Baz/)
  end
end
