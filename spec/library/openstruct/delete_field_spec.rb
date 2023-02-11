require_relative '../../spec_helper'
require 'ostruct'

describe "OpenStruct#delete_field" do
  before :each do
    @os = OpenStruct.new(name: "John Smith", age: 70, pension: 300)
  end

  it "removes the named field from self's method/value table" do
    NATFIXME 'Implement OpenStruct#delete_field', exception: NoMethodError, message: "undefined method `delete_field'" do
      @os.delete_field(:name)
      @os[:name].should be_nil
    end
  end

  it "does remove the accessor methods" do
    NATFIXME 'Implement OpenStruct#delete_field', exception: NoMethodError, message: "undefined method `delete_field'" do
      @os.delete_field(:name)
      @os.respond_to?(:name).should be_false
      @os.respond_to?(:name=).should be_false
    end
  end
end
