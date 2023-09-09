require_relative '../../spec_helper'

describe "IO#inspect" do
  before :each do
    @name = tmp("io_inspect.txt")
    touch @name
  end

  after :each do
    @r.close if @r && !@r.closed?
    @w.close if @w && !@w.closed?
    rm_r @name
  end

  it "contains the file descriptor number" do
    NATFIXME 'Implement IO.pipe', exception: NoMethodError, message: "undefined method `pipe' for IO:Class" do
      @r, @w = IO.pipe
    end
    @r = IO.new(IO.sysopen(@name))
    @r.inspect.should include("fd #{@r.fileno}")
  end

  it "contains \"(closed)\" if the stream is closed" do
    NATFIXME 'Implement IO.pipe', exception: NoMethodError, message: "undefined method `pipe' for IO:Class" do
      @r, @w = IO.pipe
    end
    @r = IO.new(IO.sysopen(@name))
    @r.close
    @r.inspect.should include("(closed)")
  end

  it "reports IO as its Method object's owner" do
    IO.instance_method(:inspect).owner.should == IO
  end
end
