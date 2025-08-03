describe :language___FILE__, shared: true do
  before :each do
    CodeLoadingSpecs.spec_setup
    @path = File.join(CODE_LOADING_DIR, "file_fixture.rb")
  end

  after :each do
    CodeLoadingSpecs.spec_cleanup
  end

  it "equals the absolute path of a file loaded by an absolute path" do
    NATFIXME 'Support runtime require', exception: NoMethodError, message: "undefined method '#{@method}' for module Kernel" do
      @object.send(@method, @path).should be_true
      ScratchPad.recorded.should == [@path]
    end
  end

  it "equals the absolute path of a file loaded by a relative path" do
    NATFIXME 'Runtime load path', exception: LoadError, message: 'Cannot manipulate $LOAD_PATH at runtime' do
      $LOAD_PATH << "."
      Dir.chdir CODE_LOADING_DIR do
        @object.send(@method, "file_fixture.rb").should be_true
      end
      ScratchPad.recorded.should == [@path]
    end
  end
end
