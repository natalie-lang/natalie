describe :tempfile_unlink, shared: true do
  before :each do
    @tempfile = Tempfile.new("specs")
  end

  it "unlinks self" do
    @tempfile.close
    path = @tempfile.path
    NATFIXME "Implement Tempfile##{@method}", exception: NoMethodError, message: "undefined method `#{@method}'" do
      @tempfile.send(@method)
      File.should_not.exist?(path)
    end
  end
end
