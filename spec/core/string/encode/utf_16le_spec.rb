# -*- encoding: utf-8 -*-
require_relative '../../../spec_helper'

describe "String#encode - to UTF-16LE" do
  context "from US-ASCII" do
    it "returns correct String in UTF-16LE" do
      a = "\x61".force_encoding("US-ASCII") # "a" - 0x61 (ASCII)
      a.encode("UTF-16LE").should == "\x61\x00".force_encoding("UTF-16LE")
    end
  end

  context "from ASCII-8BIT" do
    context "0-127" do
      it "returns correct String in UTF-16LE" do
        b = "\x61".force_encoding("ASCII-8BIT") # "a" - 0x61 (ASCII)
        b.encode("UTF-16LE").should == "\x61\x00".force_encoding("UTF-16LE")
      end
    end

    context "128-255" do
      it "raises Encoding::UndefinedConversionError" do
        b = "\x8F".force_encoding("ASCII-8BIT") # = 128

        -> { b.encode("UTF-16LE") }.should raise_error(
          Encoding::UndefinedConversionError,
          '"\x8F" to UTF-8 in conversion from ASCII-8BIT to UTF-8 to UTF-16LE'
        )
      end
    end
  end

  context "from UTF-8" do
    context "from 1-byte UTF-8" do
      it "returns correct String in UTF-16LE" do
        a = "\x61".force_encoding("UTF-8") # "a" - U+0061 (Unicode)
        a.encode("UTF-16LE").should == "\x61\x00".force_encoding("UTF-16LE")
      end
    end

    context "from 2-bytes UTF-8" do
      it "returns correct String in UTF-16LE" do
        c = "\xC2\xA5".force_encoding("UTF-8") # "¥" - U+00A5 (Unicode, Latin-1 Supplement)
        c.encode("UTF-16LE").should == "\xA5\x00".force_encoding("UTF-16LE")
      end
    end

    context "from 3-bytes UTF-8" do
      it "returns correct String in UTF-16LE" do
        c = "\xE2\x84\x9C".force_encoding("UTF-8") # "ℜ" - U+211C (Unicode, Letterlike Symbols)
        c.encode("UTF-16LE").should == "\x1C\x21".force_encoding("UTF-16LE")
      end
    end

    context "from 4-bytes UTF-8" do
      it "returns correct String in UTF-16LE" do
        c = "\xF0\x90\x85\x87".force_encoding("UTF-8") # U+10147 (Unicode, Ancient Greek Numbers)
        c.encode("UTF-16LE").should == "\x00\xD8\x47\xDD".force_encoding("UTF-16LE")
      end
    end
  end

  context "from UTF-16LE" do
    it "returns a String in UTF-16LE with the same binary representation" do
      c = "\x00\xD8\x47\xDD".force_encoding("UTF-16LE") # U+10147
      c.encode("UTF-16LE").should == c
    end
  end
end
