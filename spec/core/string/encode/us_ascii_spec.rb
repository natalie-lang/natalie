# -*- encoding: utf-8 -*-
require_relative '../../../spec_helper'

describe "String#encode - to US-ASCII" do
  context "from US-ASCII" do
    it "returns correct String in US-ASCII" do
      a = "\x61".force_encoding("US-ASCII") # "a" - 0x61 (ASCII)

      a.encode("US-ASCII").bytes.should == [0x61]
      a.encode("US-ASCII").encoding.should == Encoding::US_ASCII
    end
  end

  context "from ASCII-8BIT" do
    context "0-127" do
      it "returns correct String in US-ASCII" do
        a = "\x61".force_encoding("ASCII-8BIT") # "a" - 0x61 (ASCII)

        a.encode("US-ASCII").bytes.should == [0x61]
        a.encode("US-ASCII").encoding.should == Encoding::US_ASCII
      end
    end

    context "128-255" do
      it "raises Encoding::UndefinedConversionError" do
        b = "\x8F".force_encoding("ASCII-8BIT") # = 128

        -> { b.encode("US-ASCII") }.should raise_error(
          Encoding::UndefinedConversionError,
          '"\x8F" to UTF-8 in conversion from ASCII-8BIT to UTF-8 to US-ASCII'
        )
      end
    end
  end

  context "from UTF-8" do
    context "from 1-byte UTF-8" do
      it "returns correct String in US-ASCII" do
        a = "\x61".force_encoding("UTF-8") # "a" - U+0061 (Unicode)

        a.encode("US-ASCII").bytes.should == [0x61]
        a.encode("US-ASCII").encoding.should == Encoding::US_ASCII
      end
    end

    context "multi-bytes UTF-8" do
      it "returns correct String in US-ASCII" do
        c = "\xC2\xA5".force_encoding("UTF-8") # "¥" - U+00A5 (Unicode, Latin-1 Supplement)
        -> { c.encode("US-ASCII") }.should raise_error(
          Encoding::UndefinedConversionError, "U+00A5 from UTF-8 to US-ASCII")

        c = "\xE2\x84\x9C".force_encoding("UTF-8") # "ℜ" - U+211C (Unicode, Letterlike Symbols)
        -> { c.encode("US-ASCII") }.should raise_error(
          Encoding::UndefinedConversionError, "U+211C from UTF-8 to US-ASCII")

        c = "\xF0\x90\x85\x87".force_encoding("UTF-8") # U+10147 (Unicode, Ancient Greek Numbers)
        -> { c.encode("US-ASCII") }.should raise_error(
          Encoding::UndefinedConversionError, "U+10147 from UTF-8 to US-ASCII")
      end
    end
  end
end
