require_relative '../spec_helper'
require_relative '../../spec/core/io/fixtures/classes'

describe 'IO#gets' do
  before :each do
    @name = tmp('io_gets')
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

  it 'can read lines larger than our block size (1024)' do
    @io.gets.should == '.' * 2050 + "\n"
    @io.gets.should == ':' * 2500 + "\n"
    @io.gets.should be_nil
  end
end

describe 'IO#ungetbyte' do
  before :each do
    @name = tmp('io_ungetbyte')
    @io = File.open(@name, 'a+')
  end

  after :each do
    @io.close
    rm_r @name
  end

  it 'converts negative values to positive bytes' do
    @io.ungetbyte(-50)
    @io.getbyte.should == 256 - 50
  end

  it 'supports negative values over 255' do
    @io.ungetbyte(-256)
    @io.getbyte.should == 0
  end

  it 'does things with big negative values' do
    @io.ungetbyte(-400)
    @io.getbyte.should == [-400].pack('C').ord

    @io.ungetbyte(-12_345_678)
    @io.getbyte.should == [-12_345_678].pack('C').ord
  end
end

describe 'IO#pos' do
  # There is little in the officials specs about interactions of buffered reads
  # with other methods. These tests add a bit more of an integration testing
  # level.
  context 'Buffered IO and interactions with other methods' do
    before :each do
      @name = tmp('io_gets')
      File.open(@name, 'w') { |fh| fh.write('.' * 200) }
      @io = File.open(@name)
    end

    after :each do
      @io.close
      rm_r @name
    end

    it 'can be negative' do
      @io.ungetbyte(0x30)
      @io.ungetbyte(0x31)
      @io.pos.should == -2
    end
  end
end

describe 'IO#pos=' do
  # There is little in the officials specs about interactions of buffered reads
  # with other methods. These tests add a bit more of an integration testing
  # level.
  context 'Buffered IO and interactions with other methods' do
    before :each do
      @name = tmp('io_gets')
      File.open(@name, 'w') { |fh| fh.write('.' * 200) }
      @io = File.open(@name)
    end

    after :each do
      @io.close
      rm_r @name
    end
    it 'cannot be set to negative values' do
      @io.ungetbyte(0x30)
      @io.ungetbyte(0x31)
      -> { @io.pos = -1 }.should raise_error(Errno::EINVAL, /Invalid argument/)
    end

    it 'completely resets the IO buffer' do
      @io.read(2)
      @io.ungetbyte(0x30)
      @io.ungetbyte(0x31)
      @io.pos = 1
      @io.getbyte.should == '.'.ord
    end
  end
end

describe 'IO#read' do
  # There is little in the officials specs about interactions of buffered reads
  # with other methods. These tests add a bit more of an integration testing
  # level.
  context 'Buffered reads and interactions with other methods' do
    before :each do
      file = File.join(__dir__, '../../spec/core/io/fixtures/lines.txt')
      @lines = File.read(file)
      @io = File.open(file, 'r')
    end

    after :each do
      @io.close if @io && !@io.closed?
    end

    it 'reads the buffer filled by ungetbyte with a limit' do
      @io.ungetbyte('X'.ord)
      @io.ungetbyte('Y'.ord)
      @io.ungetbyte('Z'.ord)

      @io.read(2).should == 'ZY'
      @io.read(3).should == "X#{@lines[0, 2]}"
    end

    it 'reads the buffer filled by ungetbyte' do
      @io.ungetbyte('X'.ord)
      @io.ungetbyte('Y'.ord)
      @io.ungetbyte('Z'.ord)

      @io.read(2).should == 'ZY'
      @io.read.should == "X#{@lines}"
    end
  end
end

describe 'IO#close' do
  it 'does not actually close underlying stdin FD' do
    fd = IO.for_fd($stdin.fileno)
    fd.close
    IO.for_fd($stdin.fileno).closed?.should == false
  end

  it 'does not actually close underlying stdout FD' do
    fd = IO.for_fd($stdout.fileno)
    fd.close
    IO.for_fd($stdout.fileno).closed?.should == false
  end

  it 'does not actually close underlying stderr FD' do
    fd = IO.for_fd($stderr.fileno)
    fd.close
    IO.for_fd($stderr.fileno).closed?.should == false
  end
end
