# frozen_string_literal: true

require_relative '../spec_helper'
require 'zlib'

describe 'Zlib' do
  LARGE_TEXT_PATH = File.expand_path('../support/large_text.txt', __dir__)
  LARGE_ZLIB_INPUT_PATH = File.expand_path('../support/large_zlib_input.txt', __dir__)

  describe 'Deflate' do
    it 'deflates' do
      deflated = Zlib::Deflate.deflate('hello world')
      deflated.should == "x\x9C\xCBH\xCD\xC9\xC9W(\xCF/\xCAI\x01\x00\x1A\v\x04]".b
    end

    it 'deflates large inputs' do
      input = 'x' * 100_000
      deflated = Zlib::Deflate.deflate(input)
      deflated.bytes.should == [120, 156, 237, 193, 49, 1, 0, 0, 0, 194, 160, 218, 139, 111, 13, 15, 160, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 87, 3, 126, 42, 37, 186]

      input = File.read(LARGE_TEXT_PATH)
      deflated = Zlib::Deflate.deflate(input, Zlib::BEST_COMPRESSION)
      deflated.should == File.read(LARGE_ZLIB_INPUT_PATH).force_encoding('ASCII-8BIT')
    end

    it 'deflates in chunks' do
      inflated = 'xyz' * 100
      zstream = Zlib::Deflate.new
      inflated.chars.each_slice(50) do |chunk|
        zstream << chunk.join
      end
      deflated = zstream.finish
      zstream.close
      deflated.bytes.should == "x\x9C\xAB\xA8\xAC\xAA\x18E\xC4!\x00a\xAF\x8D\xCD".b.bytes
    end

    it 'returns itself in the streaming interface' do
      zstream = Zlib::Deflate.new
      zstream << 'foo' << 'bar'
      zstream.finish.should == Zlib.deflate('foobar')
    end

    it 'raises an error after the stream is closed' do
      zstream = Zlib::Deflate.new
      zstream.close
      -> { zstream << 'hello' }.should raise_error(Zlib::Error, 'stream is not ready')
    end
  end

  describe 'Inflate' do
    it 'inflates' do
      deflated = "x\x9C\xCBH\xCD\xC9\xC9W(\xCF/\xCAI\x01\x00\x1A\v\x04]".b
      Zlib::Inflate.inflate(deflated).should == 'hello world'
    end

    it 'inflates large inputs' do
      deflated = [120, 156, 237, 193, 49, 1, 0, 0, 0, 194, 160, 218, 139, 111, 13, 15, 160, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 87, 3, 126, 42, 37, 186].map(&:chr).join.b
      inflated = Zlib::Inflate.inflate(deflated)
      inflated.should == 'x' * 100_000

      deflated = File.read(LARGE_ZLIB_INPUT_PATH).b
      inflated = Zlib::Inflate.inflate(deflated)
      inflated.should == File.read(LARGE_TEXT_PATH)
    end

    it 'inflates in chunks' do
      deflated = "x\x9C\xCBH\xCD\xC9\xC9W(\xCF/\xCAI\x01\x00\x1A\v\x04]".b
      zstream = Zlib::Inflate.new                                                                                                                                                             
      deflated.chars.each_slice(5) do |chunk|
        zstream << chunk.join
      end
      inflated = zstream.finish                                                                                                                                                               
      zstream.close                                                                                                                                                                           
      inflated.should == 'hello world'
    end

    it 'returns itself in the streaming interface' do
      deflated = "x\x9C\xCBH\xCD\xC9\xC9W(\xCF/\xCAI\x01\x00\x1A\v\x04]".b
      zstream = Zlib::Inflate.new
      deflated.chars.each_slice(5) do |chunk|
        (zstream << chunk.join).should == zstream
      end
      inflated = zstream.finish
      zstream.close
      inflated.should == 'hello world'
    end

    it 'raises an error after the stream is closed' do
      zstream = Zlib::Inflate.new
      zstream.close
      -> { zstream << 'hello' }.should raise_error(Zlib::Error, 'stream is not ready')
    end
  end
end
