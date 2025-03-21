require_relative '../../spec_helper'
require 'csv'

describe 'CSV#each' do
  it 'yields each row' do
    c = CSV.new('1,2,3')
    yielded = []
    c.each { |x| yielded << x }

    yielded.should == [%w[1 2 3]]
  end

  it 'yields nothing for empty file' do
    c = CSV.new('')
    yielded = []
    c.each { |x| yielded << x }

    yielded.should == []
  end

  context 'with headers: true' do
    it 'yields nothing for file with only headers' do
      c = CSV.new('a,b,c', headers: true)
      yielded = []
      c.each { |x| yielded << x }

      yielded.should == []
    end

    it 'yields instances of CSV::Row with headers' do
      c = CSV.new("a,b,c\n1,2,3", headers: true)
      yielded = []
      c.each { |x| yielded << x }

      yielded.count.should == 1

      row = yielded.first

      row.headers.should == %w[a b c]
      row.fields.should == %w[1 2 3]
    end
  end
end
