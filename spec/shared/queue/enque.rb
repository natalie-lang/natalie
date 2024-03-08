describe :queue_enq, shared: true do
  it "adds an element to the Queue" do
    q = @object.call
    q.size.should == 0
    NATFIXME "Implement Queue##{@method}", exception: NoMethodError, message: "undefined method `#{@method}' for an instance of Queue" do
      q.send @method, Object.new
      q.size.should == 1
      q.send @method, Object.new
      q.size.should == 2
    end
  end

  it "is an error for a closed queue" do
    q = @object.call
    NATFIXME "Implement Queue#close", exception: NoMethodError, message: "undefined method `close' for an instance of Queue" do
      q.close
      -> {
        q.send @method, Object.new
      }.should raise_error(ClosedQueueError)
    end
  end
end
