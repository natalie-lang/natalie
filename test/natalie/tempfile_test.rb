require_relative '../spec_helper'
require 'tempfile'

describe 'Tempfile' do
  describe '.create' do
    it 'returns a new File object that you can write to' do
      temp = Tempfile.create('foo')
      temp.should be_an_instance_of(File)
      temp.write('hello world')
      temp.close
      File.read(temp.path).should == 'hello world'
    end
  end

  describe '#path' do
    it 'returns nil if the file is unlinked' do
      temp = Tempfile.new('foo')
      temp.unlink
      temp.path.should be_nil
    end
  end
end
