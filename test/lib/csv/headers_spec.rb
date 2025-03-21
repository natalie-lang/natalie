require_relative '../../spec_helper'
require 'csv'

describe 'CSV#headers' do
  it 'is nil if headers will not be used' do
    csv = CSV.new("a,b,c\n1,2,3", headers: false)

    csv.headers.should == nil
  end

  it 'is true if headers will but not yet have been read' do
    csv = CSV.new("a,b,c\n1,2,3", headers: true)

    csv.headers.should == true
  end

  it 'is the actual headers after they have been read' do
    csv = CSV.new("a,b,c\n1,2,3", headers: true)

    csv.readline

    csv.headers.should == %w[a b c]
  end
end
