require_relative '../spec_helper'
require_relative '../support/nat_binary'

describe 'special globals' do
  describe '$0' do
    it 'returns the path to the initial ruby script executed/compiled' do
      `#{NAT_BINARY} test/support/dollar_zero.rb`.strip.should == '"test/support/dollar_zero.rb"'
      `#{NAT_BINARY} -c test/tmp/dollar_zero test/support/dollar_zero.rb`
      `test/tmp/dollar_zero`.strip.should == '"test/support/dollar_zero.rb"'
    end
  end

  describe '$exe' do
    it 'returns the path to the compiled binary exe' do
      `#{NAT_BINARY} test/support/dollar_exe.rb`.strip.should =~ /natalie/ # usually something like: /tmp/natalie20210119-2242842-eystnh
      `#{NAT_BINARY} -c test/tmp/dollar_exe test/support/dollar_exe.rb`
      `test/tmp/dollar_exe`.strip.should == '"test/tmp/dollar_exe"'
    end
  end

  describe '$>' do
    it 'returns $stdout' do
      $>.should == $stdout
    end

    it 'points to $stdout even if $stdout is changed' do
      stdout_was = $stdout
      filename = tmp('stdout.log')
      File.open(filename, 'w') do |file|
        $stdout = file
        $>.should == file
        # reset
        $stdout = stdout_was
        $> = file
        $stdout.should == file
        $>.should == file
      end
    ensure
      rm_r filename
      $stdout = stdout_was
    end
  end
end
