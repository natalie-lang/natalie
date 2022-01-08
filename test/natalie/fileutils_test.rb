require_relative '../spec_helper'
require 'fileutils'

describe 'FileUtils' do
  describe '.rm_rf' do
    it 'removes files and directories' do
      path = File.expand_path('../tmp/dir', __dir__)
      `mkdir -p #{path}/dir2`
      `touch #{path}/file1.txt`
      `touch #{path}/file2.txt`
      `touch #{path}/dir2/file3.md`

      FileUtils.rm_rf(path)

      File.exist?("#{path}/file1.txt").should be_false
      File.exist?("#{path}/file2.txt").should be_false
      File.exist?("#{path}/dir2/file3.md").should be_false
      File.exist?("#{path}/dir2").should be_false
      File.exist?("#{path}").should be_false
    end

    it 'does not error when given a non-existent path' do
      -> { FileUtils.rm_rf('path/to/absolutely/nothing') }.should_not raise_error
    end
  end
end
