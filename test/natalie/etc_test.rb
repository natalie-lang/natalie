require_relative '../../spec/spec_helper'
require 'etc'

describe 'Etc.systmpdir' do
  it 'returns the path to a writable and readable directory' do
    tmpdir = Etc.systmpdir
    File.directory?(tmpdir).should be_true
    File.writable?(tmpdir).should be_true
  end

  it 'is not affected by $TMPDIR envvar' do
    tmpdir = Etc.systmpdir
    env_tmpdir = ENV['TMPDIR']
    ENV['TMPDIR'] = __dir__
    Etc.systmpdir.should == tmpdir
    Etc.systmpdir.should_not == __dir__
  ensure
    if env_tmpdir
      ENV['TMPDIR'] = env_tmpdir
    else
      ENV.delete('TMPDIR')
    end
  end
end
