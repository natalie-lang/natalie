require_relative '../../spec_helper'
require 'csv'

describe 'CSV#header_row?' do
  it 'is false if headers are not enabled' do
    csv = CSV.new("a,b,c\n1,2,3", headers: false)

    csv.header_row?.should == false
  end

  it 'is false if headers are already read' do
    csv = CSV.new("a,b,c\n1,2,3", headers: true)

    csv.readline

    csv.header_row?.should == false
  end

  it 'is true if next row will be a header row' do
    csv = CSV.new("a,b,c\n1,2,3", headers: true)

    csv.header_row?.should == true
  end
end
