require_relative '../../../spec_helper'

describe "Array#pack with empty format" do
  it "returns an empty String" do
    [1, 2, 3].pack("").should == ""
  end

  # NATFIXME: we don't have US-ASCII encoding yet
  #it "returns a String with US-ASCII encoding" do
  #  [1, 2, 3].pack("").encoding.should == Encoding::US_ASCII
  #end
end
