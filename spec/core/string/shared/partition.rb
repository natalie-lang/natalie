require_relative '../../../spec_helper'
require_relative '../fixtures/classes'

describe :string_partition, shared: true do
  it "returns String instances when called on a subclass" do
    StringSpecs::MyString.new("hello").send(@method, "l").each do |item|
      item.should be_an_instance_of(String)
    end

    StringSpecs::MyString.new("hello").send(@method, "x").each do |item|
      item.should be_an_instance_of(String)
    end

    StringSpecs::MyString.new("hello").send(@method, /l./).each do |item|
      item.should be_an_instance_of(String)
    end
  end
end
