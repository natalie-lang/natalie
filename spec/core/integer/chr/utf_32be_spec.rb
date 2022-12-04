# -*- encoding: utf-8 -*-
require_relative '../../../spec_helper'

describe "Integer#chr - UTF-32BE" do
  it "returns String in UTF-32BE encoding" do
    0x7F.chr("UTF-32BE").encoding.should == Encoding::UTF_32BE
  end

  it "returns correct binary representation for codepoints 0-FF" do
    0x7F.chr("UTF-32BE").b.should == "\x00\x00\x00\x7F".b
  end

  it "returns correct binary representation for codepoints 1000-FFFF" do
    0x1234.chr("UTF-32BE").b.should == "\x00\x00\x12\x34".b
  end

  it "returns correct binary representation for codepoints 100000-10FFFF" do
    0x103456.chr("UTF-32BE").b.should == "\x00\x10\x34\x56".b
  end

  it "raises when codepoint isn't supported by the encoding" do
    -> { 0x1F0000.chr("UTF-32BE") }.should raise_error(RangeError, "invalid codepoint 0x1F0000 in UTF-32BE")
    -> { 0xD800.chr("UTF-32BE") }.should raise_error(RangeError, "invalid codepoint 0xD800 in UTF-32BE")

    -> { 0x0100000000.chr("UTF-32BE") }.should raise_error(RangeError, "4294967296 out of char range")
  end
end
