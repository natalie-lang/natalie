describe :queue_close, shared: true do
  it "may be called multiple times" do
    q = @object.call
    NATFIXME 'Implement Queue#close', exception: NoMethodError, message: "undefined method `close' for an instance of Queue" do
      q.close
      q.closed?.should be_true
      q.close # no effect
      q.closed?.should be_true
    end
  end

  it "returns self" do
    q = @object.call
    NATFIXME 'Implement Queue#close', exception: NoMethodError, message: "undefined method `close' for an instance of Queue" do
      q.close.should == q
    end
  end
end
