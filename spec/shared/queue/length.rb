describe :queue_length, shared: true do
  it "returns the number of elements" do
    q = @object.call
    NATFIXME "Implement Queue##{@method}", exception: NoMethodError, message: "undefined method `#{@method}' for an instance of Queue" do
      q.send(@method).should == 0
      q << Object.new
      q << Object.new
      q.send(@method).should == 2
    end
  end
end
