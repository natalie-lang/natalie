require_relative '../../spec_helper'
require 'csv'

describe "CSV#each" do
  it "yields each row" do
    c = CSV.new("1,2,3")
    yielded = []
    c.each { |x| yielded << x }

    yielded.should == [["1", "2", "3"]]
  end

  it "yields nothing for empty file" do
    c = CSV.new("")
    yielded = []
    c.each { |x| yielded << x }

    yielded.should == []
  end
end
