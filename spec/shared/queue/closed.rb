describe :queue_closed?, shared: true do
  it "returns false initially" do
    queue = @object.call
    NATFIXME 'Implement Queue#closed?', exception: NoMethodError, message: "undefined method `closed?' for an instance of Queue" do
      queue.closed?.should be_false
    end
  end

  it "returns true when the queue is closed" do
    queue = @object.call
    NATFIXME 'Implement Queue#close', exception: NoMethodError, message: "undefined method `close' for an instance of Queue" do
      queue.close
      queue.closed?.should be_true
    end
  end
end
