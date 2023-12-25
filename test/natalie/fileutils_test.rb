require_relative '../spec_helper'
require 'fileutils'

describe 'FileUtils' do
  TMP_DIR = File.expand_path('../tmp/fileutils', __dir__)

  before do
    `rm -rf #{TMP_DIR}`
    `mkdir -p #{TMP_DIR}`
  end

  describe '.rm_rf' do
    it 'removes files and directories' do
      `mkdir -p #{TMP_DIR}/dir2`
      `touch #{TMP_DIR}/file1.txt`
      `touch #{TMP_DIR}/file2.txt`
      `touch #{TMP_DIR}/dir2/file3.md`

      FileUtils.rm_rf(TMP_DIR)

      File.exist?("#{TMP_DIR}/file1.txt").should be_false
      File.exist?("#{TMP_DIR}/file2.txt").should be_false
      File.exist?("#{TMP_DIR}/dir2/file3.md").should be_false
      File.exist?("#{TMP_DIR}/dir2").should be_false
      File.exist?("#{TMP_DIR}").should be_false
    end

    it 'does not error when given a non-existent path' do
      -> {
        FileUtils.rm_rf(File.join(TMP_DIR, 'path/to/absolutely/nothing'))
      }.should_not raise_error
    end
  end

  describe '.mkdir_p' do
    it 'creates nested directories' do
      path = File.join(TMP_DIR, 'foo/bar/baz')
      FileUtils.mkdir_p(path)
      File.directory?(path).should be_true
    end

    it 'raises an error when the path is a file' do
      foo = File.join(TMP_DIR, 'foo')
      touch foo
      -> {
        FileUtils.mkdir_p(foo)
      }.should raise_error(Errno::EEXIST, /File exists/)

      bar = File.join(foo, 'bar')
      -> {
        FileUtils.mkdir_p(bar)
      }.should raise_error(Errno::EEXIST, /File exists/)
    end

    it 'sets the directory mode to 0775 by default' do
      path = File.join(TMP_DIR, 'foo/bar/baz')
      FileUtils.mkdir_p(path)
      File.stat(path).mode.to_s(8).should == '40775'
    end

    it 'accepts a mode argument' do
      path = File.join(TMP_DIR, 'foo/bar/baz')
      FileUtils.mkdir_p(path, mode: 0777)
      File.stat(path).mode.to_s(8).should == '40777'

      FileUtils.rm_rf(File.join(TMP_DIR, 'foo'))

      path = File.join(TMP_DIR, 'foo/bar/baz')
      FileUtils.mkdir_p(path, mode: 0700)
      File.stat(path).mode.to_s(8).should == '40700'

      path = File.split(path).first
      File.stat(path).mode.to_s(8).should == '40700'
      path = File.split(path).first
      File.stat(path).mode.to_s(8).should == '40700'
    end
  end
end
