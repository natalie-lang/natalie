require_relative '../spec_helper'

describe 'Dir' do
  describe '.each_child' do
    it 'raises an error if given a bad path' do
      -> { Dir.each_child('this_directory_does_not_exist') {  } }.should raise_error(SystemCallError)
    end

    it 'yields each entry in the named directory except . and ..' do
      entries = []
      Dir.each_child(File.expand_path('../support', __dir__)) do |entry|
        entries << entry
      end
      entries.sort.should == [
        'compare_rubies.rb',
        'dollar_exe.rb',
        'dollar_zero.rb',
        'file.txt',
        'formatters',
        'require_sub1.rb',
        'require_sub2.rb',
        'require_sub3.rb',
        'require_sub4.foo',
        'require_sub4.foo.rb',
        'spec.rb',
      ]
    end

    it 'returns an Enumerator when no block is given' do
      dir = Dir.each_child(File.expand_path('../support', __dir__))
      dir.should be_an_instance_of(Enumerator)
      dir.to_a.sort.should == [
        'compare_rubies.rb',
        'dollar_exe.rb',
        'dollar_zero.rb',
        'file.txt',
        'formatters',
        'require_sub1.rb',
        'require_sub2.rb',
        'require_sub3.rb',
        'require_sub4.foo',
        'require_sub4.foo.rb',
        'spec.rb',
      ]
    end
  end

  describe '.pwd' do
    it 'returns the current working directory' do
      Dir.pwd.should == File.expand_path('../..', __dir__)
    end
  end
end
