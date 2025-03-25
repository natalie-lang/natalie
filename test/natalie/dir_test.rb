require_relative '../spec_helper'

describe 'Dir' do
  describe '.chdir' do
    it 'Changes the directory back when using throw/catch' do
      cwd_before = Dir.pwd
      cwd_in_block = nil
      catch(:foo) do
        Dir.chdir(__dir__) do
          cwd_in_block = Dir.pwd
          throw :foo
        end
      end
      cwd_in_block.should == __dir__
      Dir.pwd.should == cwd_before
    end
  end

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

  describe '.glob' do
    it 'works with absolute paths' do
      root = File.expand_path('../support', __dir__)
      pattern = File.join(root, '*.txt')
      names = %w[file.txt large_text.txt large_zlib_input.txt].map { |n| File.join(root, n) }
      Dir[pattern].sort.should == names
    end
  end
end

describe '__dir__' do
  it 'returns the directory containing the source file' do
    __dir__.should =~ %r{test/natalie$}
  end

  it 'does not get confused by requiring another file' do
    true.should == true # some expression to cache Transform @file
    require('optparse')
    __dir__.should =~ %r{test/natalie$}
  end
end
