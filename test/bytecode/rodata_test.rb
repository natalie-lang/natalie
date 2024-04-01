# -*- encoding: binary -*-

require_relative '../spec_helper'
require 'natalie/compiler/bytecode/ro_data'

describe 'Bytecode::RoData' do
  describe 'write access' do
    before :each do
      @rodata = Natalie::Compiler::Bytecode::RoData.new
    end

    it 'can store multiple strings' do
      @rodata.add('foo')
      @rodata.add('quux')

      @rodata.bytesize.should == 9
      @rodata.to_s.should == "\x03foo\x04quux"
    end

    it 'returns the index of the stored item' do
      @rodata.add('foo').should == 0
      @rodata.add('quux').should == 4
    end

    it 'returns the index of an existing item on duplicates' do
      @rodata.add('foo')
      @rodata.add('quux')

      @rodata.add('foo').should == 0
      @rodata.add('quux').should == 4
    end
  end

  describe 'read access' do
    before :each do
      @rodata = Natalie::Compiler::Bytecode::RoData.load("\x03foo\x04quux")
    end

    it 'has loaded the given data' do
      @rodata.bytesize.should == 9
      @rodata.to_s.should == "\x03foo\x04quux"
    end

    it 'can load data from the correct offsets' do
      @rodata.get(0).should == 'foo'
      @rodata.get(4).should == 'quux'
    end

    it 'can convert and cache the results' do
      @rodata.get(0, convert: :to_sym).should == :foo
      @rodata.get(4, convert: :to_sym).should == :quux

      # Remove the internal buffer to validate the cache
      @rodata.instance_variable_set(:@buffer, ''.b)
      @rodata.get(0, convert: :to_sym).should == :foo
      @rodata.get(4, convert: :to_sym).should == :quux

      # Reading a string will fail
      -> {
        @rodata.get(4)
      }.should raise_error(NoMethodError, "undefined method `unpack1' for nil")
    end

    it 'can convert results to Encoding objects' do
      utf8_index = @rodata.add(Encoding::UTF_8.to_s)
      binary_index = @rodata.add(Encoding::BINARY.to_s)

      @rodata.get(utf8_index, convert: :to_encoding).should == Encoding::UTF_8
      @rodata.get(binary_index, convert: :to_encoding).should == Encoding::BINARY
    end
  end
end
