require_relative '../spec_helper'

describe 'shell out' do
  describe 'backticks' do
    it 'executes the given string and returns the result from stdout' do
      `echo foo`.should == "foo\n"
    end

    it 'sets $? to the return value' do
      `echo foo`
      $?.exitstatus.should == 0
      `which not_a_thing_on_the_path 2>&1`
      $?.exitstatus.should == 1
    end

    it 'works with interpolated strings' do
      `echo #{1 + 1}`.should == "2\n"
    end
  end
end
