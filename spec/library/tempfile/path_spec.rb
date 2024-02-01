require_relative '../../spec_helper'
require 'tempfile'

describe "Tempfile#path" do
  before :each do
    NATFIXME 'Support argument in Tempfile#initialize', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
      @tempfile = Tempfile.new("specs", tmp(""))
    end
  end

  after :each do
    NATFIXME 'Implement Tempfile#close!', exception: NoMethodError, message: "undefined method `close!'" do
      @tempfile.close!
    end
  end

  it "returns the path to the tempfile" do
    NATFIXME 'Support argument in Tempfile#initialize', exception: NoMethodError, message: "undefined method `path' for nil" do
      tmpdir = tmp("")
      path = @tempfile.path

      platform_is :windows do
        # on Windows, both types of slashes are OK,
        # but the tmp helper always uses '/'
        path.gsub!('\\', '/')
      end

      path[0, tmpdir.length].should == tmpdir
      path.should include("specs")
    end
  end
end
