describe :partially_closable_sockets, shared: true do
  it "if the write end is closed then the other side can read past EOF without blocking" do
    @s1.write("foo")
    NATFIXME 'Implement IO#close_write', exception: NoMethodError, message: "undefined method `close_write' for an instance of #{@s1.class}" do
      @s1.close_write
      @s2.read("foo".size + 1).should == "foo"
    end
  end

  it "closing the write end ensures that the other side can read until EOF" do
    @s1.write("hello world")
    NATFIXME 'Implement IO#close_write', exception: NoMethodError, message: "undefined method `close_write' for an instance of #{@s1.class}" do
      @s1.close_write
      @s2.read.should == "hello world"
    end
  end
end
