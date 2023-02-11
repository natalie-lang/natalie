require_relative '../../spec_helper'
require 'ostruct'

describe "OpenStruct#initialize" do
  it "is private" do
    NATFIXME 'not a responsibility of OpenStruct, see #998', exception: SpecFailedException do
      OpenStruct.should have_private_instance_method(:initialize)
    end
  end
end
