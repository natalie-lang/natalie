require_relative "../spec_helper"
require "stringio"

describe "StringIO" do
  describe "#getc" do
    it "encodes result as UTF-8" do
      io = StringIO.new("example")
      io.getc.encoding.should == Encoding::UTF_8
    end
  end
end
