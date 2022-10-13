require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe "Numeric#numerator" do
  before :each do
    @obj = NumericSpecs::Subclass.new
  end

  it "calls self#to_r then returns the #numerator" do
    result = mock("Numeric#to_r result")
    result.should_receive(:numerator).and_return(12)
    @obj.should_receive(:to_r).and_return(result)

    @obj.numerator.should == 12
  end
end
