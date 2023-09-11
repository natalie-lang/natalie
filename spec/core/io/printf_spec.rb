require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe "IO#printf" do
  before :each do
    @name = tmp("io_printf.txt")
    @io = new_io @name
    NATFIXME 'Implement IO#sync=', exception: NoMethodError, message: "undefined method `sync='" do
      @io.sync = true
    end
  end

  after :each do
    @io.close if @io
    rm_r @name
  end

  it "calls #to_str to convert the format object to a String" do
    obj = mock("printf format")
    NATFIXME '#to_str calls should probably be done in Kernel.sprintf', exception: NoMethodError, message: "undefined method `chars'" do
      obj.should_receive(:to_str).and_return("%s")

      @io.printf obj, "printf"
      File.read(@name).should == "printf"
    end
  end

  it "writes the #sprintf formatted string" do
    @io.printf "%d %s", 5, "cookies"
    File.read(@name).should == "5 cookies"
  end

  it "raises IOError on closed stream" do
    -> { IOSpecs.closed_io.printf("stuff") }.should raise_error(IOError)
  end
end
