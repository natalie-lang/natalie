require_relative '../spec_helper'
require 'stringio'

describe 'StringIO' do
  describe '#getc' do
    it 'encodes result as UTF-8' do
      io = StringIO.new('example')
      io.getc.encoding.should == Encoding::UTF_8
    end
  end

  # NATFIXME: Remove if the spec from ruby/spec can be used
  describe '#external_encoding' do
    it 'gets the encoding of the underlying String' do
      io = StringIO.new
      io.set_encoding Encoding::BINARY
      io.external_encoding.should == Encoding::BINARY
    end

    it "changes to match string if string's encoding is changed" do
      io = StringIO.new
      io.string.force_encoding(Encoding::BINARY)
      io.external_encoding.should == Encoding::BINARY
    end

    it 'does not set the encoding of its buffer string if the string is frozen' do
      str = 'foo'.freeze
      enc = str.encoding
      io = StringIO.new(str)
      io.set_encoding Encoding::BINARY
      io.external_encoding.should == Encoding::BINARY
      str.encoding.should == enc
    end
  end
end
