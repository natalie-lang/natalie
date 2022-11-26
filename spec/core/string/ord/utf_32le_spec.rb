# -*- encoding: utf-8 -*-
require_relative '../../../spec_helper'

describe "String#ord and UTF-32LE" do
  def utf_32le(*bytes)
    bytes.pack("C*").force_encoding("UTF-32LE")
  end

  it "returns codepoint that is representing by 1 significant byte" do
    utf_32le(0x10, 0x00, 0x00, 0x00).ord.should == 0x10
  end

  it "returns codepoint that is representing by 2 significant bytes" do
    utf_32le(0xFF, 0x10, 0x00, 0x00).ord.should == 0x10FF
  end

  it "returns codepoint that is representing by 3 significant bytes" do
    utf_32le(0x77, 0xFF, 0x10, 0x00).ord.should == 0x10FF77
  end

  it "raises ArgumentError when encoding is not valid" do
    string = "\xFF\xFF\x11\x00".force_encoding("UTF-32LE") # U+10FFFF + 1
    -> { string.ord }.should raise_error(ArgumentError, "invalid byte sequence in UTF-32LE")

    string = "\xFF\xDF\x00".force_encoding("UTF-32LE") # U+DFFF
    -> { string.ord }.should raise_error(ArgumentError, "invalid byte sequence in UTF-32LE")
  end
end
