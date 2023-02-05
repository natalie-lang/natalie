require_relative '../../spec_helper'
require_relative 'fixtures/classes'
require_relative 'shared/inspect'

describe "Struct#inspect" do
  # NATFIXME: Include the name of the created class
  xit "returns a string representation showing members and values" do
    car = StructClasses::Car.new('Ford', 'Ranger')
    car.inspect.should == '#<struct StructClasses::Car make="Ford", model="Ranger", year=nil>'
  end

  it_behaves_like :struct_inspect, :inspect
end
