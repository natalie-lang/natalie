require_relative '../../spec_helper'
require 'csv'

describe 'CSV#lineno' do
  it 'is 0 initially' do
    csv = CSV.new("a,b,c\n1,2,3")

    csv.lineno.should == 0
  end

  it 'is 1 after one row read' do
    csv = CSV.new("a,b,c\n1,2,3")

    csv.readline

    csv.lineno.should == 1
  end

  it 'increases only once when line-end characters are in fields' do
    csv = CSV.new(%Q[a,"multi\n\n\nline",c\n1,2,3])

    csv.readline

    csv.lineno.should == 1
  end
end
