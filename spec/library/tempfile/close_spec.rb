require_relative '../../spec_helper'
require 'tempfile'

describe "Tempfile#close when passed no argument or [false]" do
  before :each do
    NATFIXME 'Support second argument in Tempfile#initialize', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
      @tempfile = Tempfile.new("specs", tmp(""))
    end
  end

  after :each do
    NATFIXME 'Implement Tempfile#close!', exception: NoMethodError, message: "undefined method `close!'" do
      @tempfile.close!
    end
  end

  it "closes self" do
    NATFIXME 'Support second argument in Tempfile#initialize', exception: NoMethodError, message: "undefined method `close' for nil" do
      @tempfile.close
      @tempfile.closed?.should be_true
    end
  end

  it "does not unlink self" do
    NATFIXME 'Support second argument in Tempfile#initialize', exception: NoMethodError, message: "undefined method `path' for nil" do
      path = @tempfile.path
      @tempfile.close
      File.should.exist?(path)
    end
  end
end

describe "Tempfile#close when passed [true]" do
  before :each do
    NATFIXME 'Support second argument in Tempfile#initialize', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
      @tempfile = Tempfile.new("specs", tmp(""))
    end
  end

  it "closes self" do
    NATFIXME 'Support second argument in Tempfile#initialize', exception: NoMethodError, message: "undefined method `close' for nil" do
      @tempfile.close(true)
      @tempfile.closed?.should be_true
    end
  end

  it "unlinks self" do
    NATFIXME 'Support second argument in Tempfile#initialize', exception: NoMethodError, message: "undefined method `path' for nil" do
      path = @tempfile.path
      @tempfile.close(true)
      File.should_not.exist?(path)
    end
  end
end

describe "Tempfile#close!" do
  before :each do
    NATFIXME 'Support second argument in Tempfile#initialize', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
      @tempfile = Tempfile.new("specs", tmp(""))
    end
  end

  it "closes self" do
    NATFIXME 'Support second argument in Tempfile#initialize', exception: NoMethodError, message: "undefined method `close!' for nil" do
      @tempfile.close!
      @tempfile.closed?.should be_true
    end
  end

  it "unlinks self" do
    NATFIXME 'Support second argument in Tempfile#initialize', exception: NoMethodError, message: "undefined method `path' for nil" do
      path =  @tempfile.path
      @tempfile.close!
      File.should_not.exist?(path)
    end
  end
end
