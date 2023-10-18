require_relative '../spec_helper'
require_relative '../../spec/core/io/fixtures/classes'

# Upstream pull request: https://github.com/ruby/spec/pull/1077
describe "IO#autoclose" do
  before :each do
    @io = IOSpecs.io_fixture "lines.txt"
  end

  after :each do
    @io.autoclose = true unless @io.closed?
    @io.close unless @io.closed?
  end

  it "can be set to true" do
    @io.autoclose = true
    @io.should.autoclose?
  end

  it "can be set to false" do
    @io.autoclose = false
    @io.should_not.autoclose?
  end

  it "can be set to any truthy value" do
    @io.autoclose = 42
    @io.should.autoclose?
  end

  it "can be set multple times" do
    @io.autoclose = true
    @io.should.autoclose?

    @io.autoclose = false
    @io.should_not.autoclose?

    @io.autoclose = true
    @io.should.autoclose?
  end

  it "cannot be queried on a closed IO object" do
    @io.close
    -> { @io.autoclose? }.should raise_error(IOError, /closed stream/)
  end

  it "cannot be set on a closed IO object" do
    @io.close
    -> { @io.autoclose = false }.should raise_error(IOError, /closed stream/)
  end
end

describe "IO#gets" do
  before :each do
    @name = tmp("io_gets")
    File.open(@name, 'w') do |fh|
      fh.write('.' * 2050 + "\n")
      fh.write(':' * 2500 + "\n")
    end
    @io = File.open(@name)
  end

  after :each do
    @io.close
    rm_r @name
  end

  it "can read lines larger than our block size (1024)" do
    @io.gets.should == '.' * 2050 + "\n"
    @io.gets.should == ':' * 2500 + "\n"
    @io.gets.should be_nil
  end
end

describe "IO#pwrite" do
  before :each do
    @name = tmp('io_pwrite')
    @io = File.open(@name, 'w')
  end

  after :each do
    @io.close
    rm_r @name
  end

  it 'calls to_s on the object to be written' do
    data = mock('data')
    data.should_receive(:to_s).and_return('x')
    @io.pwrite(data, 0)
    File.read(@name).should == 'x'
  end

  it 'calls to_int on the offset' do
    offset = mock('offset')
    offset.should_receive(:to_int).and_return(0)
    @io.pwrite('x', offset)
    File.read(@name).should == 'x'
  end
end

describe "IO#ungetbyte" do
  before :each do
    @name = tmp('io_pwrite')
    @io = File.open(@name, 'a+')
  end

  after :each do
    @io.close
    rm_r @name
  end

  it "converts negative values to positive bytes" do
    @io.ungetbyte(-50)
    @io.getbyte.should == 256 - 50
  end

  it "supports negative values over 255" do
    @io.ungetbyte(-256)
    @io.getbyte.should == 0
  end

  it "does things with big negative values" do
    @io.ungetbyte(-400)
    @io.getbyte.should == [-400].pack('C').ord

    @io.ungetbyte(-12345678)
    @io.getbyte.should == [-12345678].pack('C').ord
  end
end

describe "IO#read" do
  # There is little in the officials specs about interactions of buffered reads
  # with other methods. These tests add a bit more of an integration testing
  # level.
  context "Buffered reads and interactions with other methods" do
    before :each do
      file = File.join(__dir__, '../../spec/core/io/fixtures/lines.txt')
      @lines = File.read(file)
      @io = File.open(file, 'r')
    end

    after :each do
      @io.close if @io && !@io.closed?
    end

    it "reads the buffer filled by ungetbyte" do
      @io.ungetbyte('X'.ord)
      @io.ungetbyte('Y'.ord)
      @io.ungetbyte('Z'.ord)

      @io.read(2).should == 'ZY'
      @io.read.should == "X#{@lines}"
    end
  end
end
