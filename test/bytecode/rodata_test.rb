# -*- encoding: binary -*-

require_relative '../spec_helper'
require 'natalie/compiler/bytecode_ro_data'

describe 'BytecodeRoData' do
  describe 'write access' do
    before :each do
      @rodata = Natalie::Compiler::BytecodeRoData.new
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
      @rodata = Natalie::Compiler::BytecodeRoData.load("\x03foo\x04quux")
    end

    it 'has loaded the given data' do
      @rodata.bytesize.should == 9
      @rodata.to_s.should == "\x03foo\x04quux"
    end

    it 'can load data from the correct offsets' do
      @rodata.get(0).should == 'foo'
      @rodata.get(4).should == 'quux'
    end
  end
end
