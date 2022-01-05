require_relative '../../spec_helper'

describe "String.allocate" do
  it "returns an instance of String" do
    str = String.allocate
    str.should be_an_instance_of(String)
  end

  it "returns a fully-formed String" do
    str = String.allocate
    str.size.should == 0
    str << "more"
    str.should == "more"
  end

  # NATFIXME: bug in spec, see https://github.com/ruby/spec/pull/909
  it "returns a binary String" do
    String.allocate.encoding.should == Encoding::BINARY
  end
end
