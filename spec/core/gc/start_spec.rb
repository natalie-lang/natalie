require_relative '../../spec_helper'

describe "GC.start" do
  it "always returns nil" do
    GC.start.should == nil
    GC.start.should == nil
  end

  it "accepts keyword arguments" do
    NATFIXME 'accept keyword argument (or just ignore them?)', exception: ArgumentError, message: 'wrong number of arguments (given 1, expected 0)' do
      GC.start(full_mark: true, immediate_sweep: true).should == nil
    end
  end
end
