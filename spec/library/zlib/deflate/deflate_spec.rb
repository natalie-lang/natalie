require 'zlib'
require_relative '../../../spec_helper'

describe "Zlib::Deflate.deflate" do
  it "deflates some data" do
    data = Array.new(10,0).pack('C*')

    zipped = Zlib::Deflate.deflate data

    zipped.should == [120, 156, 99, 96, 128, 1, 0, 0, 10, 0, 1].pack('C*')
  end

  it "deflates lots of data" do
    data = "\000" * 32 * 1024

    zipped = Zlib::Deflate.deflate data

    zipped.should == ([120, 156, 237, 193, 1, 1, 0, 0] +
                      [0, 128, 144, 254, 175, 238, 8, 10] +
                      Array.new(31, 0) +
                      [24, 128, 0, 0, 1]).pack('C*')
  end

  it "deflates chunked data" do
    random_generator = Random.new(0)
    deflated         = ''

    Zlib::Deflate.deflate(random_generator.bytes(20000)) do |chunk|
      deflated << chunk
    end

    NATFIXME 'deflates chunked data', exception: SpecFailedException do
      deflated.length.should == 20016
    end
  end
end

describe "Zlib::Deflate#deflate" do
  before :each do
    @deflator = Zlib::Deflate.new
  end

  it "deflates some data" do
    data = "\000" * 10

    NATFIXME 'Add Zlib::FINISH', exception: NameError, message: 'uninitialized constant Zlib::FINISH' do
      zipped = @deflator.deflate data, Zlib::FINISH
      @deflator.finish

      zipped.should == [120, 156, 99, 96, 128, 1, 0, 0, 10, 0, 1].pack('C*')
    end
  end

  it "deflates lots of data" do
    data = "\000" * 32 * 1024

    NATFIXME 'Add Zlib::FINISH', exception: NameError, message: 'uninitialized constant Zlib::FINISH' do
      zipped = @deflator.deflate data, Zlib::FINISH
      @deflator.finish

      zipped.should == ([120, 156, 237, 193, 1, 1, 0, 0] +
                        [0, 128, 144, 254, 175, 238, 8, 10] +
                        Array.new(31, 0) +
                        [24, 128, 0, 0, 1]).pack('C*')
    end
  end

  it "has a binary encoding" do
    NATFIXME 'Implement Zlib::Deflate#deflate', exception: NoMethodError, message: "undefined method `deflate'" do
      @deflator.deflate("").encoding.should == Encoding::BINARY
      @deflator.finish.encoding.should == Encoding::BINARY
    end
  end
end

describe "Zlib::Deflate#deflate" do

  before :each do
    @deflator         = Zlib::Deflate.new
    @random_generator = Random.new(0)
    @original         = ''
    @chunks           = []
  end

  describe "without break" do

    before do
      2.times do
        @input = @random_generator.bytes(20000)
        @original << @input
        NATFIXME 'Implement Zlib::Deflate#deflate', exception: NoMethodError, message: "undefined method `deflate'" do
          @deflator.deflate(@input) do |chunk|
            @chunks << chunk
          end
        end
      end
    end

    it "deflates chunked data" do
      @deflator.finish
      NATFIXME 'deflates chunked data', exception: SpecFailedException do
        @chunks.map { |chunk| chunk.length }.should == [16384, 16384]
      end
    end

    it "deflates chunked data with final chunk" do
      final = @deflator.finish
      NATFIXME 'deflates chunked data', exception: SpecFailedException do
        final.length.should == 7253
      end
    end

    it "deflates chunked data without errors" do
      final = @deflator.finish
      @chunks << final
      NATFIXME 'Implement Zlib.inflate', exception: NoMethodError, message: "undefined method `inflate'" do
        @original.should == Zlib.inflate(@chunks.join)
      end
    end

  end

  describe "with break" do
    before :each do
      @input = @random_generator.bytes(20000)
      NATFIXME 'Implement Zlib::Deflate#deflate', exception: NoMethodError, message: "undefined method `deflate'" do
        @deflator.deflate(@input) do |chunk|
          @chunks << chunk
          break
        end
      end
    end

    it "deflates only first chunk" do
      @deflator.finish
      NATFIXME 'deflates chunked data', exception: SpecFailedException do
        @chunks.map { |chunk| chunk.length }.should == [16384]
      end
    end

    it "deflates chunked data with final chunk" do
      final = @deflator.finish
      NATFIXME 'deflates chunked data', exception: SpecFailedException do
        final.length.should == 3632
      end
    end

    it "deflates chunked data without errors" do
      final = @deflator.finish
      @chunks << final
      NATFIXME 'Implement Zlib.inflate', exception: NoMethodError, message: "undefined method `inflate'" do
        @input.should == Zlib.inflate(@chunks.join)
      end
    end

  end
end
