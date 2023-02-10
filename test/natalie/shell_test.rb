require_relative '../spec_helper'

describe 'shell out' do
  describe 'backticks' do
    it 'executes the given string and returns the result from stdout' do
      `echo foo`.should == "foo\n"
    end

    it 'returns an empty string if there is no output' do
      `sh -c 'exit'`.should == ''
    end

    it 'sets $? to the return value' do
      `echo foo`
      $?.exitstatus.should == 0
      $?.should == 0
      $?.to_i.should == 0
      `sh -c 'exit 10'`
      $?.exitstatus.should == 10
      $?.should == 2560
      $?.to_i.should == 2560
    end

    it 'works with interpolated strings' do
      `echo #{1 + 1}`.should == "2\n"
    end
  end

  describe 'spawn' do
    it 'executes the given command with arguments and returns the pid' do
      pid = spawn('sh', '-c', '')
      pid.should be_an_instance_of(Integer)
    end

    xit 'executes the given command string by splitting it apart' do
      pid = spawn('sh -c ""')
      pid.should be_an_instance_of(Integer)
    end

    it 'throws the correct error message when the first argument is not a string' do
      -> { spawn(1) }.should raise_error(TypeError, 'no implicit conversion of Integer into String')
    end
  end

  describe 'Process.wait' do
    it 'waits for the given process pid to complete and sets $?' do
      pid = spawn('sleep', '1')
      result = Process.wait(pid)
      result.should == pid
      $?.should == 0
    end
  end
end
