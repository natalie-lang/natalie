require_relative '../../spec_helper'
require 'tempfile'

describe "Tempfile#_close" do
  before :each do
    @tempfile = Tempfile.new("specs")
  end

  after :each do
    NATFIXME 'Implement Tempfile#close!', exception: NoMethodError, message: "undefined method `close!'" do
      @tempfile.close!
    end
  end

  it "is protected" do
    NATFIXME 'Implement Tempfile#_close', exception: SpecFailedException do
      Tempfile.should have_protected_instance_method(:_close)
    end
  end

  it "closes self" do
    NATFIXME 'Implement Tempfile#_close', exception: NoMethodError, message: "undefined method `_close'" do
      @tempfile.send(:_close)
      @tempfile.closed?.should be_true
    end
  end
end
