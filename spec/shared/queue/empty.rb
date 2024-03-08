describe :queue_empty?, shared: true do
  it "returns true on an empty Queue" do
    queue = @object.call
    NATFIXME 'Implement Queue#empty?', exception: NoMethodError, message: "undefined method `empty?' for an instance of Queue" do
      queue.empty?.should be_true
    end
  end

  it "returns false when Queue is not empty" do
    queue = @object.call
    NATFIXME 'Implement Queue#<<', exception: NoMethodError, message: "undefined method `<<' for an instance of Queue" do
      queue << Object.new
      queue.empty?.should be_false
    end
  end
end
