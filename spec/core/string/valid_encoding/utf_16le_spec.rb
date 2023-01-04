# -*- encoding: utf-8 -*-
require_relative '../../../spec_helper'

describe "String#valid_encoding? and UTF-16LE" do
  def utf16le(*bytes)
    bytes.pack("C*").force_encoding("UTF-16LE")
  end

  context "when one 16-bits code unit" do
    it "is valid if in range 0000..D7FF" do
      utf16le(0x00, 0x00).valid_encoding?.should == true
      utf16le(0xFF, 0xD7).valid_encoding?.should == true
    end

    it "is valid if in range E000..FFFF" do
      utf16le(0x00, 0xE0).valid_encoding?.should == true
      utf16le(0xFF, 0xFF).valid_encoding?.should == true
    end
  end

  context "when two 16-bits code units" do
    it "is valid if 1st code unit is in range D800..DBFF and the second one is in range DC00..DFFF" do
      utf16le(0x00, 0xD8, 0x00, 0xDC).valid_encoding?.should == true
      utf16le(0xFF, 0xDB, 0x00, 0xDC).valid_encoding?.should == true

      utf16le(0x00, 0xD8, 0xFF, 0xDF).valid_encoding?.should == true
      utf16le(0xFF, 0xDB, 0xFF, 0xDF).valid_encoding?.should == true
    end

    it "is invalid if 1st code unit is in range D800..DBFF but the second one isn't in range DC00..DFFF" do
      utf16le(0x00, 0xD8, 0xFF, 0xDB).valid_encoding?.should == false # the second code unit - DC00-1 - within D800-DFFF
      utf16le(0x00, 0xD8, 0xFF, 0xA1).valid_encoding?.should == false # the second code unit - A1FF - out of D800-DFFF
      utf16le(0x00, 0xD8, 0xFF, 0xE1).valid_encoding?.should == false # the second code unit - E1FF - out of D800-DFFF
    end

    it "is invalid if 1st code unit isn't in range D800..DBFF but the second one is in range DC00..DFFF" do
      utf16le(0x00, 0xDC, 0x00, 0xDC).valid_encoding?.should == false # the first code unit - DBFF+1 - within D800-DFFF
      utf16le(0xBB, 0xAA, 0x00, 0xDC).valid_encoding?.should == false # the first code unit - AABB - out of D800..DFFF
      utf16le(0xFF, 0xEE, 0x00, 0xDC).valid_encoding?.should == false # the first code unit - EEFF - out of D800..DFFF
    end
  end

  it "is invalid if there are 3 bytes" do
    utf16le(0x00, 0xD8, 0x00, 0xDC).valid_encoding?.should == true

    utf16le(0x00, 0xD8, 0x00).valid_encoding?.should == false
    utf16le(0xD8, 0x00, 0xDC).valid_encoding?.should == false
  end

  it "is invalid if there is 1 byte" do
    utf16le(0xFF, 0xD7).valid_encoding?.should == true

    utf16le(0xFF).valid_encoding?.should == false
    utf16le(0xD7).valid_encoding?.should == false
  end
end
