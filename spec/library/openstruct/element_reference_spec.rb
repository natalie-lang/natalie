require_relative '../../spec_helper'
require "ostruct"

describe "OpenStruct#[]" do
  before :each do
    @os = OpenStruct.new
  end

  it "returns the associated value" do
    NATFIXME 'Implement OpenStruct settrs', exception: NoMethodError, message: "undefined method `foo='" do
      @os.foo = 42
      @os[:foo].should == 42
    end
  end
end
