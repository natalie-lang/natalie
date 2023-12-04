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

    it 'can chop a character (this uses EncdoingObject::prev_char)' do
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
end
