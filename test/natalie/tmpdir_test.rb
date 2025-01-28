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
