describe :tempfile_length, shared: true do
  before :each do
    @tempfile = Tempfile.new("specs")
  end

  after :each do
    @tempfile.close!
  end

  it "returns the size of self" do
    @tempfile.send(@method).should eql(0)
    @tempfile.print("Test!")
    NATFIXME 'Do we need to flush the buffer?', exception: SpecFailedException do
      @tempfile.send(@method).should eql(5)
    end
  end

  it "returns the size of self even if self is closed" do
    @tempfile.print("Test!")
    @tempfile.close
    @tempfile.send(@method).should eql(5)
  end
end
