# -*- encoding: utf-8 -*-
require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe "IO.binread" do
  before :each do
    @internal = Encoding.default_internal

    @fname = tmp('io_read.txt')
    @contents = "1234567890"
    touch(@fname) { |f| f.write @contents }
  end

  after :each do
    rm_r @fname
    Encoding.default_internal = @internal
  end

  it "reads the contents of a file" do
    IO.binread(@fname).should == @contents
  end

  it "reads the contents of a file up to a certain size when specified" do
    IO.binread(@fname, 5).should == @contents.slice(0..4)
  end

  it "reads the contents of a file from an offset of a specific size when specified" do
    IO.binread(@fname, 5, 3).should == @contents.slice(3, 5)
  end

  it "returns a String in BINARY encoding" do
    IO.binread(@fname).encoding.should == Encoding::BINARY
  end

  it "returns a String in BINARY encoding regardless of Encoding.default_internal" do
    Encoding.default_internal = Encoding::EUC_JP
    IO.binread(@fname).encoding.should == Encoding::BINARY
  end

  it "raises an ArgumentError when not passed a valid length" do
    -> { IO.binread @fname, -1 }.should raise_error(ArgumentError)
  end

  it "raises an Errno::EINVAL when not passed a valid offset" do
    -> { IO.binread @fname, 0, -1  }.should raise_error(Errno::EINVAL)
  end

  ruby_version_is "3.3" do
    # https://bugs.ruby-lang.org/issues/19630
    it "warns about deprecation given a path with a pipe" do
      cmd = "|echo ok"
      NATFIXME 'Pipes in binread', exception: NotImplementedError, message: 'no support for pipe in IO.binread' do
        -> {
          IO.binread(cmd)
        }.should complain(/IO process creation with a leading '\|'/)
      end
    end
  end
end
