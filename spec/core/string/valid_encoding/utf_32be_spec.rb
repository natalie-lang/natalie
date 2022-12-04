# -*- encoding: utf-8 -*-
require_relative '../../../spec_helper'

describe "String#valid_encoding? and UTF-32BE" do
  def utf_32be(*bytes)
    bytes.pack("C*").force_encoding("UTF-32BE")
  end

  it "is valid in range 0-U+D800" do
    utf_32be(0x00, 0x00, 0x00, 0x00).valid_encoding?.should == true
    utf_32be(0x00, 0x00, 0xD7, 0xFF).valid_encoding?.should == true # U+D800 - 1 => D7FF
  end

  it "is invalid in range U+D800...U+DFFF" do
    utf_32be(0x00, 0x00, 0xD8, 0x00).valid_encoding?.should == false # U+D800
    utf_32be(0x00, 0x00, 0xDF, 0xFF).valid_encoding?.should == false # U+DFFF
  end

  it "is valid in range U+DFFF..U+10FFFF" do
    utf_32be(0x00, 0x00, 0xE0, 0x00).valid_encoding?.should == true # U+DFFF + 1 => E000
    utf_32be(0x00, 0x10, 0xFF, 0xFF).valid_encoding?.should == true # U+10FFFF
  end

  it "is invalid in range U+10FFFF.." do
    utf_32be(0x00, 0x11, 0xFF, 0xFF).valid_encoding?.should == false # U+10FFFF + 1 = 11FFFF
    utf_32be(0xFF, 0xFF, 0xFF, 0xFF).valid_encoding?.should == false
  end
end
