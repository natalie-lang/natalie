require_relative '../../spec_helper'
require "ostruct"

describe "OpenStruct#marshal_load when passed [Hash]" do
  it "defines methods based on the passed Hash" do
    os = OpenStruct.new
    NATFIXME 'Implement OpenStruct#marshal_load', exception: NoMethodError, message: "undefined method `marshal_load'" do
      os.send :marshal_load, age: 20, name: "John"

      os.age.should eql(20)
      os.name.should == "John"
    end
  end
end
