# encoding: utf-8

require 'stringio'
require_relative '../spec_helper'

describe 'StringIO' do
  describe '#each_byte' do
    it 'splits multibyte characters into individual bytes' do
      str = '😉🤷'
      io = StringIO.new(str)
      io.each_byte.to_a.should == str.bytes
    end
  end

  describe '#read' do
    context 'with a buffer' do
      it 'tries to convert a buffer argument with #to_str' do
        buffer = mock('to_str')
        str = +''
        buffer.should_receive(:to_str).and_return(str)
        io = StringIO.new('foobar')
        io.read(nil, buffer)
        buffer.to_str.should == 'foobar'
      end

      it 'clears the buffer on an empty read' do
        buffer = +'buffer'
        io = StringIO.new('foobar')
        io.read(0, buffer).should equal(buffer)
        buffer.should == ''
      end

      it 'clears the buffer on an EOF read' do
        buffer = +'buffer'
        io = StringIO.new('foobar')
        io.read
        io.should.eof?
        io.read(100, buffer).should be_nil
        buffer.should == ''
      end

      it 'raises a TypeError if the buffer is not a String and does not respond to to_str' do
        buffer = mock('')
        io = StringIO.new('foobar')
        -> { io.read(nil, buffer) }.should raise_error(TypeError, 'no implicit conversion of MockObject into String')
      end

      it 'raises a TypeError if the buffer is not a String and #to_str does not return a String' do
        buffer = mock('to_str')
        buffer.should_receive(:to_str).and_return(:symbol)
        io = StringIO.new('foobar')
        -> { io.read(nil, buffer) }.should raise_error(
                     TypeError,
                     "can't convert MockObject to String (MockObject#to_str gives Symbol)",
                   )
      end

      it 'raises an error if no bytes could be read' do
        buffer = mock('')
        io = StringIO.new('foobar')
        -> { io.read(0, buffer) }.should raise_error(TypeError, 'no implicit conversion of MockObject into String')
      end
    end
  end
end
