# -*- encoding: utf-8 -*-
require_relative '../../../spec_helper'

describe "Integer#chr - UTF-32LE" do
  it "returns String in UTF-32LE encoding" do
    0x7F.chr("UTF-32LE").encoding.should == Encoding::UTF_32LE
  end

  it "returns correct binary representation for codepoints 0-FF" do
    0x7F.chr("UTF-32LE").b.should == "\x7F\x00\x00\x00".b
  end

  it "returns correct binary representation for codepoints 1000-FFFF" do
    0x1234.chr("UTF-32LE").b.should == "\x34\x12\x00\x00".b
  end

  it "returns correct binary representation for codepoints 100000-10FFFF" do
    0x103456.chr("UTF-32LE").b.should == "\x56\x34\x10\x00".b
  end

  it "raises when codepoint isn't supported by the encoding" do
    -> { 0x1F0000.chr("UTF-32LE") }.should raise_error(RangeError, "invalid codepoint 0x1F0000 in UTF-32LE")
    -> { 0xD800.chr("UTF-32LE") }.should raise_error(RangeError, "invalid codepoint 0xD800 in UTF-32LE")

    -> { 0x0100000000.chr("UTF-32LE") }.should raise_error(RangeError, "4294967296 out of char range")
  end
end
