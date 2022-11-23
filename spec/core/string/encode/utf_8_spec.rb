# -*- encoding: utf-8 -*-
require_relative '../../../spec_helper'

describe "String#encode - to UTF-8" do
  context "from US-ASCII" do
    it "returns correct String in UTF-8" do
      a = "\x61".force_encoding("US-ASCII") # "a" - 0x61 (ASCII)

      a.encode("UTF-8").bytes.should == [0x61]
      a.encode("UTF-8").encoding.should == Encoding::UTF_8
    end
  end

  context "from ASCII-8BIT" do
    context "0-127" do
      it "returns correct String in UTF-8" do
        b = "\x61".force_encoding("ASCII-8BIT") # "a" - 0x61 (ASCII)

        b.encode("UTF-8").bytes.should == [0x61]
        b.encode("UTF-8").encoding.should == Encoding::UTF_8
      end
    end

    context "128-255" do
      it "raises Encoding::UndefinedConversionError" do
        b = "\x8F".force_encoding("ASCII-8BIT") # = 128

        -> { b.encode("UTF-8") }.should raise_error(
          Encoding::UndefinedConversionError, '"\x8F" from ASCII-8BIT to UTF-8')
      end
    end
  end

  context "from UTF-8" do
    it "returns correct String in UTF-8" do
      a = "\x61".force_encoding("UTF-8") # "a" - U+0061 (Unicode)
      a.encode("UTF-8").bytes.should == [0x61]
      a.encode("UTF-8").encoding.should == Encoding::UTF_8

      a = "\xC2\xA5".force_encoding("UTF-8") # "¥" - U+00A5 (Unicode, Latin-1 Supplement)
      a.encode("UTF-8").bytes.should == [0xC2, 0xA5]
      a.encode("UTF-8").encoding.should == Encoding::UTF_8

      a = "\xE2\x84\x9C".force_encoding("UTF-8") # "ℜ" - U+211C (Unicode, Letterlike Symbols)
      a.encode("UTF-8").bytes.should == [0xE2, 0x84, 0x9C]
      a.encode("UTF-8").encoding.should == Encoding::UTF_8

      a = "\xF0\x90\x85\x87".force_encoding("UTF-8") # U+10147 (Unicode, Ancient Greek Numbers)
      a.encode("UTF-8").bytes.should == [0xF0, 0x90, 0x85, 0x87]
      a.encode("UTF-8").encoding.should == Encoding::UTF_8
    end
  end
end
