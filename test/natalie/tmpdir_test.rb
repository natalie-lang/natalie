require_relative '../../spec/spec_helper'
require 'etc'
require 'tmpdir'

describe 'Dir.tmpdir' do
  it 'is affected by $TMPDIR envvar' do
    env_tmpdir = ENV['TMPDIR']
    ENV['TMPDIR'] = __dir__
    Dir.tmpdir.should_not == Etc.systmpdir
    Dir.tmpdir.should == __dir__
  ensure
    if env_tmpdir
      ENV['TMPDIR'] = env_tmpdir
    else
      ENV.delete('TMPDIR')
    end
  end
end

describe 'Dir.mktmpdir' do
  it 'creates a private directory without a block' do
    path = Dir.mktmpdir
    File.directory?(path).should == true
    (File.stat(path).mode & 0777).should == 0700
  ensure
    FileUtils.rm_rf(path) if path
  end

  it 'returns the block result and removes the directory and its files' do
    path = nil
    result =
      Dir.mktmpdir do |dir|
        path = dir
        File.write(File.join(dir, 'test'), 'temporary')
        :result
      end
    result.should == :result
    File.exist?(path).should == false
  end

  it 'cleans up when the block raises' do
    path = nil
    -> do
      Dir.mktmpdir do |dir|
        path = dir
        raise 'temporary failure'
      end
    end.should raise_error(RuntimeError, 'temporary failure')
    File.exist?(path).should == false
  end
end
