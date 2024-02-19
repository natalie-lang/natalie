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
end
