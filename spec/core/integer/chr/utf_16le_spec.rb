# -*- encoding: utf-8 -*-
require_relative '../../../spec_helper'

describe "Integer#chr - UTF-16LE" do
  it "returns String in UTF-16LE encoding" do
    0x7F.chr("UTF-16LE").encoding.should == Encoding::UTF_16LE
  end

  it "returns correct binary representation for codepoints U+0000-U+D7FF" do
    0x7F.chr("UTF-16LE").b.should == "\x7F\x00".b
    0xD7FF.chr("UTF-16LE").b.should == "\xFF\xD7".b
  end

  it "returns correct binary representation for codepoints U+E000-U+FFFF" do
    0xE000.chr("UTF-16LE").b.should == "\x00\xE0".b
    0xFFFF.chr("UTF-16LE").b.should == "\xFF\xFF".b
  end

  it "returns correct binary representation for codepoints U+010000-U+10FFFF" do
    0x010000.chr("UTF-16LE").b.should == "\x00\xD8\x00\xDC".b
    0x10FFFF.chr("UTF-16LE").b.should == "\xFF\xDB\xFF\xDF".b
  end

  it "raises RangeError for codepoints U+D800-DFFF" do
    -> { 0xD800.chr("UTF-16LE") }.should raise_error(RangeError, "invalid codepoint 0xD800 in UTF-16LE")
    -> { 0xDA77.chr("UTF-16LE") }.should raise_error(RangeError, "invalid codepoint 0xDA77 in UTF-16LE")
    -> { 0xDFFF.chr("UTF-16LE") }.should raise_error(RangeError, "invalid codepoint 0xDFFF in UTF-16LE")
  end

  it "raises RangeError for codepoints > U+10FFFF" do
    -> { 0xAF0000.chr("UTF-16LE") }.should raise_error(RangeError, "invalid codepoint 0xAF0000 in UTF-16LE")
    -> { 0x0100000000.chr("UTF-16LE") }.should raise_error(RangeError, "4294967296 out of char range")
  end
end
