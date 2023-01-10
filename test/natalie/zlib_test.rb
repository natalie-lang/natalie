require_relative '../spec_helper'
require 'zlib'

describe 'Zlib' do
  describe 'Deflate' do
    it 'deflates' do
      deflated = Zlib::Deflate.deflate('hello world')
      deflated.should == "x\x9C\xCBH\xCD\xC9\xC9W(\xCF/\xCAI\x01\x00\x1A\v\x04]".force_encoding('ASCII-8BIT')
    end
  end

  describe 'Inflate' do
    it 'inflates' do
      deflated = "x\x9C\xCBH\xCD\xC9\xC9W(\xCF/\xCAI\x01\x00\x1A\v\x04]".force_encoding('ASCII-8BIT')
      Zlib::Inflate.inflate(deflated).should == 'hello world'
    end
  end
end
