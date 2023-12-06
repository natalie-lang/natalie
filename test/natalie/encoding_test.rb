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
        0x61 => 0x61,
        0x8E => 0x8E,
        0xD8 => 0xA8,
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
        0x61 => 0x61,
        0x8E => 0x8E,
        0x1E80 => 0xA8,
        0xFF => 0xFF,
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
        0x61 => 0x61,
        0x8E => 0x8E,
        0x161 => 0xA8,
        0xFF => 0xFF,
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
        0x61 => 0x61,
        0x8E => 0x8E,
        0x161 => 0xA8,
        0xFF => 0xFF,
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
end
