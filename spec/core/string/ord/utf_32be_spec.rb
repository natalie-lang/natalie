# -*- encoding: utf-8 -*-
require_relative '../../../spec_helper'

describe "String#ord and UTF-32BE" do
  def utf_32be(*bytes)
    bytes.pack("C*").force_encoding("UTF-32BE")
  end

  it "returns codepoint that is representing by 1 significant byte" do
    utf_32be(0x00, 0x00, 0x00, 0x10).ord.should == 0x10
  end

  it "returns codepoint that is representing by 2 significant bytes" do
    utf_32be(0x00, 0x00, 0x10, 0xFF).ord.should == 0x10FF
  end

  it "returns codepoint that is representing by 3 significant bytes" do
    utf_32be(0x00, 0x10, 0xFF, 0x77).ord.should == 0x10FF77
  end

  it "raises ArgumentError when encoding is not valid" do
    string = "\x00\x11\xFF\xFF".force_encoding("UTF-32BE") # U+10FFFF + 1
    -> { string.ord }.should raise_error(ArgumentError, "invalid byte sequence in UTF-32BE")

    string = "\x00\x00\xDF\xFF".force_encoding("UTF-32BE") # U+DFFF
    -> { string.ord }.should raise_error(ArgumentError, "invalid byte sequence in UTF-32BE")
  end
end
