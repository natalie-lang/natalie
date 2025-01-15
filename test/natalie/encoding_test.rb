require_relative '../spec_helper'

describe 'encodings' do
  describe 'EUC-JP' do
    it 'can convert codepoints' do
      [
        0x61,
        0xA1A1,
        0x8FFEF1,
      ].each do |codepoint|
        codepoint.chr(Encoding::EUC_JP).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61     => 0x61,
        0x8EA1   => 0xFF61,
        0x8febf4 => 0x9d5f,
        0x8fede3 => 0x9fa5,
        0xA1A1   => 0x3000,
        0xA1A2   => 0x3001,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::EUC_JP).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61   => 0x61,
        0xFF61 => 0x8EA1,
        0x9d5f => 0x8febf4,
        0x9fa5 => 0x8fede3,
        0x3000 => 0xA1A1,
        0x3001 => 0xA1A2,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::EUC_JP).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0xA1A1,
        0x8FFEF1,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::EUC_JP) + codepoint.chr(Encoding::EUC_JP)
        string.encoding.should == Encoding::EUC_JP
        string.chop!
        string.encoding.should == Encoding::EUC_JP
        string.bytes.should == 'a'.encode(Encoding::EUC_JP).bytes
      end
    end
  end

  describe 'ISO-8859-1' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::ISO_8859_1).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x8E,
        0xFF => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::ISO_8859_1).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x8E,
        0xFF => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::ISO_8859_1).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::ISO_8859_1) + codepoint.chr(Encoding::ISO_8859_1)
        string.encoding.should == Encoding::ISO_8859_1
        string.chop!
        string.encoding.should == Encoding::ISO_8859_1
        string.bytes.should == 'a'.encode(Encoding::ISO_8859_1).bytes
      end
    end
  end

  describe 'ISO-8859-2' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::ISO_8859_2).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x8E,
        0xA1 => 0x104,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::ISO_8859_2).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x8E  => 0x8E,
        0x104 => 0xA1,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::ISO_8859_2).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::ISO_8859_2) + codepoint.chr(Encoding::ISO_8859_2)
        string.encoding.should == Encoding::ISO_8859_2
        string.chop!
        string.encoding.should == Encoding::ISO_8859_2
        string.bytes.should == 'a'.encode(Encoding::ISO_8859_2).bytes
      end
    end
  end

  describe 'ISO-8859-3' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::ISO_8859_3).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x8E,
        0xA1 => 0x126,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::ISO_8859_3).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x8E  => 0x8E,
        0x126 => 0xA1,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::ISO_8859_3).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::ISO_8859_3) + codepoint.chr(Encoding::ISO_8859_3)
        string.encoding.should == Encoding::ISO_8859_3
        string.chop!
        string.encoding.should == Encoding::ISO_8859_3
        string.bytes.should == 'a'.encode(Encoding::ISO_8859_3).bytes
      end
    end
  end

  describe 'ISO-8859-4' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::ISO_8859_4).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x8E,
        0xE0 => 0x101,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::ISO_8859_4).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x8E  => 0x8E,
        0x101 => 0xE0,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::ISO_8859_4).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::ISO_8859_4) + codepoint.chr(Encoding::ISO_8859_4)
        string.encoding.should == Encoding::ISO_8859_4
        string.chop!
        string.encoding.should == Encoding::ISO_8859_4
        string.bytes.should == 'a'.encode(Encoding::ISO_8859_4).bytes
      end
    end
  end

  describe 'ISO-8859-5' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::ISO_8859_5).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x8E,
        0xE0 => 0x440,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::ISO_8859_5).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x8E  => 0x8E,
        0x440 => 0xE0,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::ISO_8859_5).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::ISO_8859_5) + codepoint.chr(Encoding::ISO_8859_5)
        string.encoding.should == Encoding::ISO_8859_5
        string.chop!
        string.encoding.should == Encoding::ISO_8859_5
        string.bytes.should == 'a'.encode(Encoding::ISO_8859_5).bytes
      end
    end
  end

  describe 'ISO-8859-6' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::ISO_8859_6).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x8E,
        0xE0 => 0x640,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::ISO_8859_6).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x8E  => 0x8E,
        0x640 => 0xE0,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::ISO_8859_6).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::ISO_8859_6) + codepoint.chr(Encoding::ISO_8859_6)
        string.encoding.should == Encoding::ISO_8859_6
        string.chop!
        string.encoding.should == Encoding::ISO_8859_6
        string.bytes.should == 'a'.encode(Encoding::ISO_8859_6).bytes
      end
    end
  end

  describe 'ISO-8859-7' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::ISO_8859_7).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x8E,
        0xA8 => 0xA8,
        0xE0 => 0x3B0,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::ISO_8859_7).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x8E  => 0x8E,
        0xA8  => 0xA8,
        0x3B0 => 0xE0,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::ISO_8859_7).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::ISO_8859_7) + codepoint.chr(Encoding::ISO_8859_7)
        string.encoding.should == Encoding::ISO_8859_7
        string.chop!
        string.encoding.should == Encoding::ISO_8859_7
        string.bytes.should == 'a'.encode(Encoding::ISO_8859_7).bytes
      end
    end
  end

  describe 'ISO-8859-8' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::ISO_8859_8).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x8E,
        0xA8 => 0xA8,
        0xE0 => 0x5D0,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::ISO_8859_8).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x8E  => 0x8E,
        0xA8  => 0xA8,
        0x5D0 => 0xE0,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::ISO_8859_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::ISO_8859_8) + codepoint.chr(Encoding::ISO_8859_8)
        string.encoding.should == Encoding::ISO_8859_8
        string.chop!
        string.encoding.should == Encoding::ISO_8859_8
        string.bytes.should == 'a'.encode(Encoding::ISO_8859_8).bytes
      end
    end
  end

  describe 'ISO-8859-9' do
    before :each do
      @exceptions = {
        0xD0 => 0x11E,
        0xDD => 0x130,
        0xDE => 0x15E,
        0xF0 => 0x11F,
        0xFD => 0x131,
        0xFE => 0x15F
      }
    end

    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::ISO_8859_9).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      0x80.upto(0xFF).each do |codepoint|
        expected = @exceptions.fetch(codepoint, codepoint)
        codepoint.chr(Encoding::ISO_8859_9).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      0x80.upto(0xFF).each do |expected|
        codepoint = @exceptions.fetch(expected, expected)
        codepoint.chr(Encoding::UTF_8).encode(Encoding::ISO_8859_9).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::ISO_8859_13) + codepoint.chr(Encoding::ISO_8859_13)
        string.encoding.should == Encoding::ISO_8859_13
        string.chop!
        string.encoding.should == Encoding::ISO_8859_13
        string.bytes.should == 'a'.encode(Encoding::ISO_8859_13).bytes
      end
    end
  end

  describe 'ISO-8859-10' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::ISO_8859_10).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x8E,
        0xA8 => 0x13B,
        0xFF => 0x138,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::ISO_8859_10).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x8E  => 0x8E,
        0x13B => 0xA8,
        0x138 => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::ISO_8859_10).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::ISO_8859_10) + codepoint.chr(Encoding::ISO_8859_10)
        string.encoding.should == Encoding::ISO_8859_10
        string.chop!
        string.encoding.should == Encoding::ISO_8859_10
        string.bytes.should == 'a'.encode(Encoding::ISO_8859_10).bytes
      end
    end
  end

  describe 'ISO-8859-11' do
    before :each do
      @unmapped = [0xDB, 0xDC, 0xDD, 0xDE, 0xFC, 0xFD, 0xFE, 0xFF]
    end

    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::ISO_8859_11).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      0x80.upto(0xFF).each do |codepoint|
        next if @unmapped.include?(codepoint)

        expected = codepoint >= 0xA1 ? 0xD60 + codepoint : codepoint
        codepoint.chr(Encoding::ISO_8859_11).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end

      @unmapped.each do |codepoint|
        -> { codepoint.chr(Encoding::ISO_8859_11).encode(Encoding::UTF_8) }.should raise_error(
          Encoding::UndefinedConversionError
        )
      end
    end

    it 'can convert from UTF-8' do
      0x80.upto(0xFF).each do |expected|
        next if @unmapped.include?(expected)

        codepoint = expected >= 0xA1 ? 0xD60 + expected : expected
        codepoint.chr(Encoding::UTF_8).encode(Encoding::ISO_8859_11).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::ISO_8859_13) + codepoint.chr(Encoding::ISO_8859_13)
        string.encoding.should == Encoding::ISO_8859_13
        string.chop!
        string.encoding.should == Encoding::ISO_8859_13
        string.bytes.should == 'a'.encode(Encoding::ISO_8859_13).bytes
      end
    end
  end

  describe 'ISO-8859-13' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::ISO_8859_13).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x8E,
        0xA8 => 0xD8,
        0xFF => 0x2019,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::ISO_8859_13).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61   => 0x61,
        0x8E   => 0x8E,
        0xD8   => 0xA8,
        0x2019 => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::ISO_8859_13).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::ISO_8859_13) + codepoint.chr(Encoding::ISO_8859_13)
        string.encoding.should == Encoding::ISO_8859_13
        string.chop!
        string.encoding.should == Encoding::ISO_8859_13
        string.bytes.should == 'a'.encode(Encoding::ISO_8859_13).bytes
      end
    end
  end

  describe 'ISO-8859-14' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::ISO_8859_14).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x8E,
        0xA8 => 0x1E80,
        0xFF => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::ISO_8859_14).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61   => 0x61,
        0x8E   => 0x8E,
        0x1E80 => 0xA8,
        0xFF   => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::ISO_8859_14).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::ISO_8859_14) + codepoint.chr(Encoding::ISO_8859_14)
        string.encoding.should == Encoding::ISO_8859_14
        string.chop!
        string.encoding.should == Encoding::ISO_8859_14
        string.bytes.should == 'a'.encode(Encoding::ISO_8859_14).bytes
      end
    end
  end

  describe 'ISO-8859-15' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::ISO_8859_15).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x8E,
        0xA8 => 0x161,
        0xFF => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::ISO_8859_15).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x8E  => 0x8E,
        0x161 => 0xA8,
        0xFF  => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::ISO_8859_15).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::ISO_8859_15) + codepoint.chr(Encoding::ISO_8859_15)
        string.encoding.should == Encoding::ISO_8859_15
        string.chop!
        string.encoding.should == Encoding::ISO_8859_15
        string.bytes.should == 'a'.encode(Encoding::ISO_8859_15).bytes
      end
    end
  end

  describe 'ISO-8859-16' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::ISO_8859_16).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x8E,
        0xA8 => 0x161,
        0xFF => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::ISO_8859_16).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x8E  => 0x8E,
        0x161 => 0xA8,
        0xFF  => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::ISO_8859_16).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::ISO_8859_16) + codepoint.chr(Encoding::ISO_8859_16)
        string.encoding.should == Encoding::ISO_8859_16
        string.chop!
        string.encoding.should == Encoding::ISO_8859_16
        string.bytes.should == 'a'.encode(Encoding::ISO_8859_16).bytes
      end
    end
  end

  describe 'KOI8-R' do
    it 'can be used to draw a table' do
      "\xA4\x80\xA7".dup.force_encoding('KOI8-R').encode('UTF-8').should == '╓─╖'
    end
  end

  describe 'KOI8-U' do
    it 'can be used to draw a flipped double table with just 2 legs' do
      "\xA9\xA0\xAC".dup.force_encoding('KOI8-U').encode('UTF-8').should == '╘═╛'
    end

    it 'cannot be used to draw a regular table' do
      "\xA4\x80\xA7".dup.force_encoding('KOI8-U').encode('UTF-8').should == 'є─ї'
    end
  end

  describe 'Windows-1250' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::Windows_1250).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x17D,
        0xA8 => 0xA8,
        0xFF => 0x2D9,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::Windows_1250).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x17D => 0x8E,
        0xA8  => 0xA8,
        0x2D9 => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::Windows_1250).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::Windows_1250) + codepoint.chr(Encoding::Windows_1250)
        string.encoding.should == Encoding::Windows_1250
        string.chop!
        string.encoding.should == Encoding::Windows_1250
        string.bytes.should == 'a'.encode(Encoding::Windows_1250).bytes
      end
    end
  end

  describe 'Windows-1253' do
    it 'can convert codepoints' do
      [
        0x61,
        0xC4,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::Windows_1253).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0xC4 => 0x394,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::Windows_1253).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x394 => 0xC4,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::Windows_1253).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::Windows_1253) + codepoint.chr(Encoding::Windows_1253)
        string.encoding.should == Encoding::Windows_1253
        string.chop!
        string.encoding.should == Encoding::Windows_1253
        string.bytes.should == 'a'.encode(Encoding::Windows_1253).bytes
      end
    end
  end

  describe 'UTF-16BE' do
    it 'can convert codepoints' do
      [
        0x61,
        0x20AC,
        0x1FAFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::UTF_16BE).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61    => 0x61,
        0x20AC  => 0x20AC,
        0x8EA1  => 0x8EA1,
        0x1F4A9 => 0x1F4A9,
        0x1FAFF => 0x1FAFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_16BE).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61    => 0x61,
        0x20AC  => 0x20AC,
        0x8EA1  => 0x8EA1,
        0x1F4A9 => 0x1F4A9,
        0x1FAFF => 0x1FAFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::UTF_16BE).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x20AC,
        0x8EA1,
        0x1F4A9,
        0x1FAFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::UTF_16BE) + codepoint.chr(Encoding::UTF_16BE)
        string.encoding.should == Encoding::UTF_16BE
        string.chop!
        string.encoding.should == Encoding::UTF_16BE
        string.bytes.should == 'a'.encode(Encoding::UTF_16BE).bytes
      end

      "\xD8\x00\xDC\x00\x00\xFF".dup.force_encoding(Encoding::UTF_16BE).chop!.bytes.should == [216, 0, 220, 0]
      "\xD8\x00\xDC\x00\xFF".dup.force_encoding(Encoding::UTF_16BE).chop!.bytes.should == [216, 0, 220, 0]
    end
  end

  describe 'UTF-16LE' do
    it 'can convert codepoints' do
      [
        0x61,
        0x20AC,
        0x1FAFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::UTF_16LE).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61    => 0x61,
        0x20AC  => 0x20AC,
        0x8EA1  => 0x8EA1,
        0x1F4A9 => 0x1F4A9,
        0x1FAFF => 0x1FAFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_16LE).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61    => 0x61,
        0x20AC  => 0x20AC,
        0x8EA1  => 0x8EA1,
        0x1F4A9 => 0x1F4A9,
        0x1FAFF => 0x1FAFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::UTF_16LE).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x20AC,
        0x8EA1,
        0x1F4A9,
        0x1FAFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::UTF_16LE) + codepoint.chr(Encoding::UTF_16LE)
        string.encoding.should == Encoding::UTF_16LE
        string.chop!
        string.encoding.should == Encoding::UTF_16LE
        string.bytes.should == 'a'.encode(Encoding::UTF_16LE).bytes
      end

      "\x00\xD8\x00\xDC\xFF\x00".dup.force_encoding(Encoding::UTF_16LE).chop!.bytes.should == [0, 216, 0, 220]
      "\x00\xD8\x00\xDC\xFF".dup.force_encoding(Encoding::UTF_16LE).chop!.bytes.should == [0, 216, 0, 220]
    end
  end

  describe 'Windows-1251' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::Windows_1251).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x40B,
        0xA8 => 0x401,
        0xFF => 0x44F,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::Windows_1251).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x40B => 0x8E,
        0x401 => 0xA8,
        0x44F => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::Windows_1251).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::Windows_1251) + codepoint.chr(Encoding::Windows_1251)
        string.encoding.should == Encoding::Windows_1251
        string.chop!
        string.encoding.should == Encoding::Windows_1251
        string.bytes.should == 'a'.encode(Encoding::Windows_1251).bytes
      end
    end
  end

  describe 'Windows-1252' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::Windows_1252).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x8E => 0x17D,
        0xA8 => 0xA8,
        0xFF => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::Windows_1252).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x17D => 0x8E,
        0xA8  => 0xA8,
        0xFF  => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::Windows_1252).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8E,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::Windows_1252) + codepoint.chr(Encoding::Windows_1252)
        string.encoding.should == Encoding::Windows_1252
        string.chop!
        string.encoding.should == Encoding::Windows_1252
        string.bytes.should == 'a'.encode(Encoding::Windows_1252).bytes
      end
    end
  end

  describe 'Windows-1254' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8D,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::Windows_1254).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0xD0 => 0x11E,
        0xFF => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::Windows_1254).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'cannot convert certain codepoints to UTF-8' do
      [
        0x81,
        0x8D,
        0x8E,
        0x8F,
        0x90,
        0x9D,
        0x9E,
      ].each do |codepoint|
        -> { codepoint.chr(Encoding::Windows_1254).encode(Encoding::UTF_8) }.should raise_error(Encoding::UndefinedConversionError, /from Windows-1254 to UTF-8/)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x11E => 0xD0,
        0xFF  => 0xFF,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::Windows_1254).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8C,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::Windows_1254) + codepoint.chr(Encoding::Windows_1254)
        string.encoding.should == Encoding::Windows_1254
        string.chop!
        string.encoding.should == Encoding::Windows_1254
        string.bytes.should == 'a'.encode(Encoding::Windows_1254).bytes
      end
    end
  end

  describe 'Windows-1255' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8D,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::Windows_1255).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0xD0 => 0x5C0,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::Windows_1255).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'cannot convert certain codepoints to UTF-8' do
      [
        0x81,
        0x8D,
        0x8E,
        0x8F,
        0x90,
        0x9D,
        0x9E,
        0xFF,
      ].each do |codepoint|
        -> { codepoint.chr(Encoding::Windows_1255).encode(Encoding::UTF_8) }.should raise_error(Encoding::UndefinedConversionError, /from Windows-1255 to UTF-8/)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x5C0 => 0xD0,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::Windows_1255).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8C,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::Windows_1255) + codepoint.chr(Encoding::Windows_1255)
        string.encoding.should == Encoding::Windows_1255
        string.chop!
        string.encoding.should == Encoding::Windows_1255
        string.bytes.should == 'a'.encode(Encoding::Windows_1255).bytes
      end
    end
  end

  describe 'Windows-1256' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8D,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::Windows_1256).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x80 => 0x20AC,
        0x81 => 0x67E,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::Windows_1256).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x20AC => 0x80,
        0x67E => 0x81,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::Windows_1256).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8C,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::Windows_1256) + codepoint.chr(Encoding::Windows_1256)
        string.encoding.should == Encoding::Windows_1256
        string.chop!
        string.encoding.should == Encoding::Windows_1256
        string.bytes.should == 'a'.encode(Encoding::Windows_1256).bytes
      end
    end
  end

  describe 'Windows-1257' do
    it 'can convert codepoints' do
      [
        0x61,
        0x8D,
        0xFF,
      ].each do |codepoint|
        codepoint.chr(Encoding::Windows_1257).ord.should == codepoint
      end
    end

    it 'can convert to UTF-8' do
      {
        0x61 => 0x61,
        0x80 => 0x20AC,
        0xC0 => 0x104,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::Windows_1257).encode(Encoding::UTF_8).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'cannot convert certain codepoints to UTF-8' do
      [
        0x81,
        0x8D,
        0x8E,
        0x8F,
        0x90,
        0x9D,
        0x9E,
        0xFF,
      ].each do |codepoint|
        -> { codepoint.chr(Encoding::Windows_1255).encode(Encoding::UTF_8) }.should raise_error(Encoding::UndefinedConversionError, /from Windows-1255 to UTF-8/)
      end
    end

    it 'can convert from UTF-8' do
      {
        0x61  => 0x61,
        0x20AC => 0x80,
        0x104 => 0xC0,
      }.each do |codepoint, expected|
        codepoint.chr(Encoding::UTF_8).encode(Encoding::Windows_1257).ord.to_s(16).should == expected.to_s(16)
      end
    end

    it 'can chop a character (this uses EncodingObject::prev_char)' do
      [
        0x61,
        0x8C,
        0xFF,
      ].each do |codepoint|
        string = 'a'.encode(Encoding::Windows_1257) + codepoint.chr(Encoding::Windows_1257)
        string.encoding.should == Encoding::Windows_1257
        string.chop!
        string.encoding.should == Encoding::Windows_1257
        string.bytes.should == 'a'.encode(Encoding::Windows_1257).bytes
      end
    end
  end
end
