require_relative '../../spec_helper'

describe "IO#readbyte" do
  before :each do
    @io = File.open(__FILE__, 'r')
  end

  after :each do
    @io.close
  end

  it "reads one byte from the stream" do
    byte = @io.readbyte
    byte.should == ?r.getbyte(0)
    NATFIXME 'Convert IO#read to use FILE*', exception: SpecFailedException do
      @io.pos.should == 1
    end
  end

  it "raises EOFError on EOF" do
    @io.seek(999999)
    -> do
      @io.readbyte
    end.should raise_error EOFError
  end
end
