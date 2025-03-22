require_relative '../../spec_helper'
require 'csv'

describe 'CSV#row_sep' do
  it 'auto discovers \r if parsing' do
    csv = CSV.new("1,2\r3,4")
    csv.row_sep.should == "\r"

    csv = CSV.new("1,2\r3,4", row_sep: :auto)
    csv.row_sep.should == "\r"
  end

  it 'auto discovers \r\n if parsing' do
    csv = CSV.new("1,2\r\n3,4")
    csv.row_sep.should == "\r\n"

    csv = CSV.new("1,2\r\n3,4", row_sep: :auto)
    csv.row_sep.should == "\r\n"
  end

  it "uses \n if no row sep found" do
    csv = CSV.new('')
    csv.row_sep.should == "\n"
  end

  it 'uses supplied row sep from options' do
    csv = CSV.new('', row_sep: ';')
    csv.row_sep.should == ';'
  end

  it 'allows empty string' do
    csv = CSV.new('', row_sep: '')
    csv.row_sep.should == ''
  end

  it 'calls to_s on supplied option' do
    row_sep = mock('to_s')
    row_sep.should_receive(:to_s).and_return('to_s')
    csv = CSV.new('', row_sep: row_sep)
    csv.row_sep.should == 'to_s'
  end

  it 'raises error if #to_s does not return string' do
    row_sep = mock('to_s')
    row_sep.should_receive(:to_s).and_return(1)
    csv = CSV.new('', row_sep: row_sep)
    -> { csv.row_sep }.should raise_error(NoMethodError, "undefined method `encode' for 1:Integer")
  end

  it 'raises error if option is not string convertible' do
    row_sep = BasicObject.new
    csv = CSV.new('', row_sep: row_sep)
    -> { csv.row_sep }.should raise_error(NoMethodError, /undefined method `to_s' for/)
  end

  it 'converts row sep to encoding of data' do
    data = ''.force_encoding(Encoding::BINARY)
    csv = CSV.new(data, row_sep: ';')
    csv.row_sep.encoding.should == Encoding::BINARY
  end
end
