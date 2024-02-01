require_relative '../../spec_helper'
require 'tempfile'

describe "Tempfile#initialize" do
  before :each do
    @tempfile = Tempfile.allocate
  end

  after :each do
    NATFIXME 'Implement Tempfile#close!', exception: NoMethodError, message: "undefined method `close!' for an instance of Tempfile" do
      @tempfile.close!
    end
  end

  it "opens a new tempfile with the passed name in the passed directory" do
    NATFIXME 'Support argument in Tempfile#initialize', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
      @tempfile.send(:initialize, "basename", tmp(""))
      File.should.exist?(@tempfile.path)

      tmpdir = tmp("")
      path = @tempfile.path

      platform_is :windows do
        # on Windows, both types of slashes are OK,
        # but the tmp helper always uses '/'
        path.gsub!('\\', '/')
      end

      path[0, tmpdir.length].should == tmpdir
      path.should include("basename")
    end
  end

  platform_is_not :windows do
    it "sets the permissions on the tempfile to 0600" do
      NATFIXME 'Support argument in Tempfile#initialize', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
        @tempfile.send(:initialize, "basename", tmp(""))
        File.stat(@tempfile.path).mode.should == 0100600
      end
    end
  end

  it "accepts encoding options" do
    NATFIXME 'Support argument in Tempfile#initialize', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
      @tempfile.send(:initialize, ['shiftjis', 'yml'], encoding: 'SHIFT_JIS')
      @tempfile.external_encoding.should == Encoding::Shift_JIS
    end
  end

  it "does not try to modify the arguments" do
    NATFIXME 'Support argument in Tempfile#initialize', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
      @tempfile.send(:initialize, ['frozen'.freeze, 'txt'.freeze], encoding: Encoding::IBM437)
      @tempfile.external_encoding.should == Encoding::IBM437
    end
  end
end
