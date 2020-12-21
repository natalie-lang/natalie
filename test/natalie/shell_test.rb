require_relative '../spec_helper'

describe 'shell out' do
  describe 'backticks' do
    it 'executes the given string and returns the result from stdout' do
      `echo foo`.should == "foo\n"
    end

    it 'sets $? to the return value' do
      `echo foo`
      $?.exitstatus.should == 0
      `sh -c 'exit 10'`
      $?.exitstatus.should == 10
    end

    it 'works with interpolated strings' do
      `echo #{1 + 1}`.should == "2\n"
    end
  end
end
