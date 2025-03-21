require_relative '../spec_helper'

describe 'Process' do
  describe '.maxgroups=' do
    before :all do
      @original_maxgroups = Process.maxgroups
    end

    after :all do
      Process.maxgroups = @original_maxgroups
    end

    it 'needs an integer' do
      -> { Process.maxgroups = 'abc' }.should raise_error(TypeError, 'no implicit conversion of String into Integer')
    end

    it 'needs a positive value' do
      -> { Process.maxgroups = -1 }.should raise_error(ArgumentError, 'maxgroups -1 should be positive')

      -> { Process.maxgroups = 0 }.should raise_error(ArgumentError, 'maxgroups 0 should be positive')
    end

    it 'converts values using to_int' do
      maxgroups = mock('to_int')
      maxgroups.should_receive(:to_int).and_return(16)
      Process.maxgroups = maxgroups
      Process.maxgroups.should == 16
    end

    it 'uses the return value of to_int in the error message' do
      maxgroups = mock('to_int')
      maxgroups.should_receive(:to_int).and_return(-1)
      -> { Process.maxgroups = maxgroups }.should raise_error(ArgumentError, 'maxgroups -1 should be positive')
    end

    guard -> { File.readable?('/proc/sys/kernel/ngroups_max') } do
      it 'truncates the argument to the actual maxgroups value' do
        actual_maxgroups = Integer(File.read('/proc/sys/kernel/ngroups_max'))
        Process.maxgroups = actual_maxgroups + 1
        Process.maxgroups.should == actual_maxgroups
      end
    end
  end

  describe '.ppid' do
    before :each do
      @file = tmp('ppid')
      @exe = tmp('exe')
      @natalie = ENV.fetch('NAT_BINARY', 'bin/natalie') # Force to use Natalie, not MRI, in the subprocess
    end

    after :each do
      rm_r @file
      rm_r @exe
    end

    it 'returns the main Natalie process' do
      pid = spawn(@natalie, '-e', "File.open(#{@file.dump}, 'w') { |f| f.puts(Process.ppid) }")
      Process.wait(pid)
      $?.exitstatus.should == 0
      File.read(@file).chomp.to_i.should == pid
    end

    it 'returns this process if we create an executable' do
      pid = spawn(@natalie, '-c', @exe, '-e', "File.open(#{@file.dump}, 'w') { |f| f.puts(Process.ppid) }")
      Process.wait(pid)
      $?.exitstatus.should == 0

      pid = spawn(@exe)
      Process.wait(pid)
      $?.exitstatus.should == 0
      File.read(@file).chomp.to_i.should == Process.pid
    end
  end
end
