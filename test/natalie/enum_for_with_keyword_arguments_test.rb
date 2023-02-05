require_relative '../spec_helper'

class EnumForWrapper < String
  def each_char_ord(ord = false)
    if block_given?
      each_char { |c| yield(ord ? c.ord : c) }
    else
      enum_for(:each_char_ord, ord)
    end
  end

  def each_char_ord_kw(ord: false)
    if block_given?
      each_char { |c| yield(ord ? c.ord : c) }
    else
      enum_for(:each_char_ord_kw, ord: ord)
    end
  end
end

describe 'enum_for with argument forwarding' do
  it 'block without arguments' do
    array = []
    EnumForWrapper.new('abc').each_char_ord { |c| array << c }
    array.should == %w[a b c]
  end

  it 'block with arguments' do
    array = []
    EnumForWrapper.new('abc').each_char_ord(true) { |c| array << c }
    array.should == %w[a b c].map(&:ord)
  end

  it 'without block and without arguments' do
    EnumForWrapper.new('abc').each_char_ord.to_a.should == %w[a b c]
  end

  it 'without block and with arguments' do
    EnumForWrapper.new('abc').each_char_ord(true).to_a.should == %w[a b c].map(&:ord)
  end
end

describe 'enum_for with keyword argument forwarding' do
  it 'block without arguments' do
    array = []
    EnumForWrapper.new('abc').each_char_ord_kw { |c| array << c }
    array.should == %w[a b c]
  end

  it 'block with arguments' do
    array = []
    EnumForWrapper.new('abc').each_char_ord_kw(ord: true) { |c| array << c }
    array.should == %w[a b c].map(&:ord)
  end

  it 'without block and without arguments' do
    EnumForWrapper.new('abc').each_char_ord_kw.to_a.should == %w[a b c]
  end

  it 'without block and with arguments' do
    EnumForWrapper.new('abc').each_char_ord_kw(ord: true).to_a.should == %w[a b c].map(&:ord)
  end
end
