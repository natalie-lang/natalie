describe :queue_length, shared: true do
  it "returns the number of elements" do
    q = @object.call
    q.send(@method).should == 0
    NATFIXME 'Implement Queue#<<', exception: NoMethodError, message: "undefined method `<<' for an instance of Queue" do
      q << Object.new
      q << Object.new
      q.send(@method).should == 2
    end
  end
end
