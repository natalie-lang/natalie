# -*- encoding: utf-8 -*-
require_relative '../../../spec_helper'

describe "String#ord and UTF-16LE" do
  def utf_16le(*bytes)
    bytes.pack("C*").force_encoding("UTF-16LE")
  end

  it "returns codepoint that is represented with a single 16-bit code unit" do
    # edge cases
    utf_16le(0x00, 0x00).ord.should == 0x0000
    utf_16le(0xFF, 0xD7).ord.should == 0xD7FF
    utf_16le(0x00, 0xE0).ord.should == 0xE000
    utf_16le(0xFF, 0xFF).ord.should == 0xFFFF

    utf_16le(0xAF, 0x09).ord.should == 0x09AF
    utf_16le(0x2E, 0xF4).ord.should == 0xF42E
  end

  it "returns codepoint that is represented with two 16-bit code units" do
    # edge cases
    utf_16le(0x00, 0xD8, 0x00, 0xDC).ord.should == 0x010000
    utf_16le(0xFF, 0xDB, 0xFF, 0xDF).ord.should == 0x10FFFF

    utf_16le(0x2A, 0xD8, 0xCD, 0xDF).ord.should == 0x01ABCD
    utf_16le(0xEA, 0xDB, 0xCD, 0xDF).ord.should == 0x10ABCD
  end

  it "raises ArgumentError for a single 16-bit code unit that encodes 0xD800–0xDFFF" do
    # edge cases
    -> { utf_16le(0x00, 0xD8).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE')
    -> { utf_16le(0xFF, 0xDF).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE')

    -> { utf_16le(0x00, 0xD9).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE')
    -> { utf_16le(0x00, 0xDA).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE')
    -> { utf_16le(0xFF, 0xDA).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE')
  end

  it "raises ArgumentError for two 16-bit code units when the second code unit is out of the range 0xDC00–0xDFFF" do
    -> { utf_16le(0x00, 0xD8, 0xFF, 0xDB).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE') # 0xDC00-1
    -> { utf_16le(0x00, 0xD8, 0x00, 0xE0).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE') # 0xDFFF+1

    -> { utf_16le(0x00, 0xD8, 0x00, 0xD0).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE')
    -> { utf_16le(0x00, 0xD8, 0x00, 0x80).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE')
    -> { utf_16le(0x00, 0xD8, 0x00, 0xEF).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE')
    -> { utf_16le(0x00, 0xD8, 0x00, 0xFF).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE')

    # correct base byte sequence
    utf_16le(0xFF, 0xDB, 0xFF, 0xDF).ord.should == 0x10FFFF
  end

  it "raise ArgumentError when there is 1 byte" do
    -> { utf_16le(0x00).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE')
    -> { utf_16le(0x77).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE')
    -> { utf_16le(0x7F).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE')
    -> { utf_16le(0xF0).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE')
    -> { utf_16le(0xF7).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE')
    -> { utf_16le(0xFF).ord }.should raise_error(ArgumentError, 'invalid byte sequence in UTF-16LE')
  end
end

