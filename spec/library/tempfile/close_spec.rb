require_relative '../../spec_helper'
require 'tempfile'

describe "Tempfile#close when passed no argument or [false]" do
  before :each do
    @tempfile = Tempfile.new("specs", tmp(""))
  end

  after :each do
    NATFIXME 'Implement Tempfile#close!', exception: NoMethodError, message: "undefined method `close!'" do
      @tempfile.close!
    end
    @tempfile.unlink # NATFIXME: Remove this once Tempfile#close! works
  end

  it "closes self" do
    @tempfile.close
    @tempfile.closed?.should be_true
  end

  it "does not unlink self" do
    path = @tempfile.path
    @tempfile.close
    File.should.exist?(path)
  end
end

describe "Tempfile#close when passed [true]" do
  before :each do
    @tempfile = Tempfile.new("specs", tmp(""))
  end

  after :each do
    @tempfile.unlink # NATFIXME: Remove this once Tempfile#close! works
  end

  it "closes self" do
    NATFIXME 'Support second argument in Tempfile#close', exception: ArgumentError, message: 'wrong number of arguments (given 1, expected 0)' do
      @tempfile.close(true)
      @tempfile.closed?.should be_true
    end
  end

  it "unlinks self" do
    path = @tempfile.path
    NATFIXME 'Support second argument in Tempfile#close', exception: ArgumentError, message: 'wrong number of arguments (given 1, expected 0)' do
      @tempfile.close(true)
      File.should_not.exist?(path)
    end
  end
end

describe "Tempfile#close!" do
  before :each do
    @tempfile = Tempfile.new("specs", tmp(""))
  end

  after :each do
    @tempfile.unlink # NATFIXME: Remove this once Tempfile#close! works
  end

  it "closes self" do
    NATFIXME 'Implement Tempfile#close!', exception: NoMethodError, message: "undefined method `close!' for an instance of Tempfile" do
      @tempfile.close!
      @tempfile.closed?.should be_true
    end
  end

  it "unlinks self" do
    path =  @tempfile.path
    NATFIXME 'Implement Tempfile#close!', exception: NoMethodError, message: "undefined method `close!' for an instance of Tempfile" do
      @tempfile.close!
      File.should_not.exist?(path)
    end
  end
end
