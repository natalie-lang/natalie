require_relative '../../spec_helper'
require "ostruct"

describe "OpenStruct#marshal_dump" do
  it "returns the method/value table" do
    os = OpenStruct.new("age" => 20, "name" => "John")
    NATFIXME 'Implement OpenStruct#marshal_dump', exception: SpecFailedException do
      os.marshal_dump.should == { age: 20, name: "John" }
    end
  end
end
