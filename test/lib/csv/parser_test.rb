require_relative '../../spec_helper'
require 'stringio'
require 'csv'

describe 'CSV::Parser' do
  it 'parse quoted columns' do
    parser = CSV::Parser.new(StringIO.new("\"foo\",\"\"\n\"bar\",\"baz\""), CSV::DEFAULT_OPTIONS)
    parser.next_line.should == ['foo', '']
    parser.next_line.should == %w[bar baz]
    parser.next_line.should == nil
  end

  it 'parse line terminators in quoted fields' do
    parser = CSV::Parser.new(StringIO.new(%Q[a,"multi\nline",b]), CSV::DEFAULT_OPTIONS)
    parser.next_line.should == ['a', "multi\nline", 'b']
  end

  it 'parse unquoted columns' do
    parser = CSV::Parser.new(StringIO.new("foo,\nbar,baz"), CSV::DEFAULT_OPTIONS)
    parser.next_line.should == ['foo', nil]
    parser.next_line.should == %w[bar baz]
    parser.next_line.should == nil
  end

  it 'ignores end line at the end' do
    parser = CSV::Parser.new(StringIO.new("foo,\nbar,baz\n"), CSV::DEFAULT_OPTIONS)
    parser.next_line.should == ['foo', nil]
    parser.next_line.should == %w[bar baz]
    parser.next_line.should == nil
  end

  it 'raise error if quote in column' do
    parser = CSV::Parser.new(StringIO.new("foo,\nbar,ba\"z"), CSV::DEFAULT_OPTIONS)
    parser.next_line.should == ['foo', nil]
    -> { parser.next_line }.should raise_error(CSV::MalformedCSVError, 'Illegal quoting in line 2.')

    parser = CSV::Parser.new(StringIO.new("foo,\nbar,ba\"z"), CSV::DEFAULT_OPTIONS.merge(liberal_parsing: true))
    parser.next_line.should == ['foo', nil]
    -> { parser.next_line }.should_not raise_error(CSV::MalformedCSVError, 'Illegal quoting in line 2.')
  end

  it 'raise error if unclosed quoted column' do
    parser = CSV::Parser.new(StringIO.new("foo,\n\"bar,baz"), CSV::DEFAULT_OPTIONS)
    parser.next_line.should == ['foo', nil]
    -> { parser.next_line }.should raise_error(CSV::MalformedCSVError, 'Unclosed quoted field in line 2.')
  end

  it 'raise error if value after end quote' do
    parser = CSV::Parser.new(StringIO.new("foo,\n\"ba\"r,baz"), CSV::DEFAULT_OPTIONS)
    parser.next_line.should == ['foo', nil]
    -> { parser.next_line }.should raise_error(
                 CSV::MalformedCSVError,
                 "Any value after quoted field isn't allowed in line 2.",
               )
  end
end
