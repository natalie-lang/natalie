require_relative '../../spec_helper'

describe "Class#initialize" do
  it "is private" do
    NATFIXME 'Implement have_private_method and Kernel#private_methods', exception: NoMethodError, message: "undefined method `have_private_method' for main" do
      Class.should have_private_method(:initialize)
    end
  end

  it "raises a TypeError when called on already initialized classes" do
    ->{
      Integer.send :initialize
    }.should raise_error(TypeError)

    ->{
      Object.send :initialize
    }.should raise_error(TypeError)
  end

  # See [redmine:2601]
  it "raises a TypeError when called on BasicObject" do
    ->{
      BasicObject.send :initialize
    }.should raise_error(TypeError)
  end

  describe "when given the Class" do
    before :each do
      @uninitialized = Class.allocate
    end

    it "raises a TypeError" do
      ->{@uninitialized.send(:initialize, Class)}.should raise_error(TypeError)
    end
  end
end
