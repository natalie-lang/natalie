require_relative '../../spec_helper'
require 'csv'

describe 'CSV#col_sep' do
  it 'uses comma as default' do
    csv = CSV.new('')
    csv.col_sep.should == ','
  end

  it 'uses supplied col sep from options' do
    csv = CSV.new('', col_sep: ';')
    csv.col_sep.should == ';'
  end

  it 'does not allow single character when parsing' do
    csv = CSV.new('', col_sep: '')
    -> { csv.col_sep }.should raise_error(ArgumentError, ":col_sep must be 1 or more characters: \"\"")

    col_sep = mock('to_s')
    col_sep.should_receive(:to_s).and_return('')
    csv = CSV.new('', col_sep: col_sep)
    -> { csv.col_sep }.should raise_error(ArgumentError, ":col_sep must be 1 or more characters: #{col_sep.inspect}")
  end

  it 'calls to_s on supplied option' do
    col_sep = mock('to_s')
    col_sep.should_receive(:to_s).and_return('to_s')
    csv = CSV.new('', col_sep: col_sep)
    csv.col_sep.should == 'to_s'
  end

  it 'raises error if #to_s does not return string' do
    col_sep = mock('to_s')
    col_sep.should_receive(:to_s).and_return(1)
    csv = CSV.new('', col_sep: col_sep)
    -> { csv.col_sep }.should raise_error(NoMethodError, "undefined method `encode' for 1:Integer")
  end

  it 'raises error if option is not string convertible' do
    col_sep = BasicObject.new
    csv = CSV.new('', col_sep: col_sep)
    -> { csv.col_sep }.should raise_error(NoMethodError, /undefined method `to_s' for/)
  end

  it 'converts col sep to encoding of data' do
    data = ''.force_encoding(Encoding::BINARY)
    csv = CSV.new(data, col_sep: ';')
    csv.col_sep.encoding.should == Encoding::BINARY
  end
end
