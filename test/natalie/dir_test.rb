require_relative '../spec_helper'

describe 'Dir' do
  describe '.each_child' do
    it 'raises an error if given a bad path' do
      -> { Dir.each_child('this_directory_does_not_exist') {} }.should raise_error(SystemCallError)
    end

    it 'yields each entry in the named directory except . and ..' do
      entries = []
      Dir.each_child(File.expand_path('../support/dir_test', __dir__)) { |entry| entries << entry }
      entries.sort.should == %w[bar.txt foo.txt]
    end

    it 'returns an Enumerator when no block is given' do
      dir = Dir.each_child(File.expand_path('../support/dir_test', __dir__))
      dir.should be_an_instance_of(Enumerator)
      dir.to_a.sort.should == %w[bar.txt foo.txt]
    end
  end

  describe '.pwd' do
    it 'returns the current working directory' do
      Dir.pwd.should == File.expand_path('../..', __dir__)
    end
  end
end
