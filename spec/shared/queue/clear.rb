describe :queue_clear, shared: true do
  it "removes all objects from the queue" do
    queue = @object.call
    queue << Object.new
    queue << 1
    queue.empty?.should be_false
    NATFIXME 'Implement Queue#clear', exception: NoMethodError, message: "undefined method `clear' for an instance of Queue" do
      queue.clear
      queue.empty?.should be_true
    end
  end

  # TODO: test for atomicity of Queue#clear
end
