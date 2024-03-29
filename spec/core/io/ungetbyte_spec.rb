require_relative '../../spec_helper'

describe "IO#ungetbyte" do
  before :each do
    @name = tmp("io_ungetbyte")
    touch(@name) { |f| f.write "a" }
    @io = new_io @name, "r"
  end

  after :each do
    @io.close unless @io.closed?
    rm_r @name
  end

  it "does nothing when passed nil" do
    @io.ungetbyte(nil).should be_nil
    @io.getbyte.should == 97
  end

  it "puts back each byte in a String argument" do
    @io.ungetbyte("cat").should be_nil
    @io.getbyte.should == 99
    @io.getbyte.should == 97
    @io.getbyte.should == 116
    @io.getbyte.should == 97
  end

  it "calls #to_str to convert the argument" do
    str = mock("io ungetbyte")
    str.should_receive(:to_str).and_return("dog")

    @io.ungetbyte(str).should be_nil
    @io.getbyte.should == 100
    @io.getbyte.should == 111
    @io.getbyte.should == 103
    @io.getbyte.should == 97
  end

  it "never raises RangeError" do
    for i in [4095, 0x4f7574206f6620636861722072616e67ff] do
      @io.ungetbyte(i).should be_nil
      @io.getbyte.should == 255
    end
  end

  # NATFIXME: Inconsistent results. We can probably fix this once we implement IO#close_read
  xit "raises IOError on stream not opened for reading" do
    -> { STDOUT.ungetbyte(42) }.should raise_error(IOError, "not opened for reading")
  end

  it "raises an IOError if the IO is closed" do
    @io.close
    -> { @io.ungetbyte(42) }.should raise_error(IOError)
  end
end
