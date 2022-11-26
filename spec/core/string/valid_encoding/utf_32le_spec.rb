# -*- encoding: utf-8 -*-
require_relative '../../../spec_helper'

describe "String#valid_encoding? and UTF-32LE" do
  def utf_32le(*bytes)
    bytes.pack("C*").force_encoding("UTF-32LE")
  end

  it "is valid in range 0-U+D800" do
    utf_32le(0x00, 0x00, 0x00, 0x00).valid_encoding?.should == true
    utf_32le(0xFF, 0xD7, 0x00, 0x00).valid_encoding?.should == true # U+D800 - 1 => D7FF
  end

  it "is invalid in range U+D800...U+DFFF" do
    utf_32le(0x00, 0xD8, 0x00, 0x00).valid_encoding?.should == false # U+D800
    utf_32le(0xFF, 0xDF, 0x00, 0x00).valid_encoding?.should == false # U+DFFF
  end

  it "is valid in range U+DFFF..U+10FFFF" do
    utf_32le(0x00, 0xE0, 0x00, 0x00).valid_encoding?.should == true # U+DFFF + 1 => E000
    utf_32le(0xFF, 0xFF, 0x10, 0x00).valid_encoding?.should == true # U+10FFFF
  end

  it "is invalid in range U+10FFFF.." do
    utf_32le(0xFF, 0xFF, 0x11, 0x00).valid_encoding?.should == false # U+10FFFF + 1 = 11FFFF
    utf_32le(0xFF, 0xFF, 0xFF, 0xFF).valid_encoding?.should == false
  end
end
