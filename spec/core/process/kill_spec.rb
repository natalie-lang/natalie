require_relative '../../spec_helper'
require_relative 'fixtures/common'

describe "Process.kill" do
  ProcessSpecs.use_system_ruby(self)

  before :each do
    @pid = Process.pid
  end

  it "raises an ArgumentError for unknown signals" do
    -> { Process.kill("FOO", @pid) }.should raise_error(ArgumentError)
  end

  it "raises an ArgumentError if passed a lowercase signal name" do
    -> { Process.kill("term", @pid) }.should raise_error(ArgumentError)
  end

  it "raises an ArgumentError if signal is not an Integer or String" do
    signal = mock("process kill signal")
    signal.should_not_receive(:to_int)

    -> { Process.kill(signal, @pid) }.should raise_error(ArgumentError)
  end

  it "raises Errno::ESRCH if the process does not exist" do
    pid = Process.spawn(*ruby_exe, "-e", "sleep 10")
    Process.kill("SIGKILL", pid)
    Process.wait(pid)
    -> {
      Process.kill("SIGKILL", pid)
    }.should raise_error(Errno::ESRCH)
  end

  it "checks for existence and permissions to signal a process, but does not actually signal it, when using signal 0" do
    Process.kill(0, @pid).should == 1
  end
end

platform_is_not :windows do
  # NATFIXME: Signalizer depends on Signal.trap (whose specs depend on Process.kill)
  #           Skip them for now, add a guard to ensure we put them back once we have
  #           Signal.trap.
  describe 'Process.kill' do
    it 'should enable the specs below and remove this one' do
      NATFIXME 'Implement Signal.trap', exception: NoMethodError do
        Signal.trap('INT') {}
      end
    end
  end

  xdescribe "Process.kill" do
    ProcessSpecs.use_system_ruby(self)

    before :each do
      @sp = ProcessSpecs::Signalizer.new
    end

    after :each do
      @sp.cleanup
    end

    it "accepts a Symbol as a signal name" do
      Process.kill(:SIGTERM, @sp.pid)
      @sp.result.should == "signaled"
    end

    it "accepts a String as signal name" do
      Process.kill("SIGTERM", @sp.pid)
      @sp.result.should == "signaled"
    end

    it "accepts a signal name without the 'SIG' prefix" do
      Process.kill("TERM", @sp.pid)
      @sp.result.should == "signaled"
    end

    it "accepts a signal name with the 'SIG' prefix" do
      Process.kill("SIGTERM", @sp.pid)
      @sp.result.should == "signaled"
    end

    it "accepts an Integer as a signal value" do
      Process.kill(15, @sp.pid)
      @sp.result.should == "signaled"
    end

    it "calls #to_int to coerce the pid to an Integer" do
      Process.kill("SIGTERM", mock_int(@sp.pid))
      @sp.result.should == "signaled"
    end
  end

  xdescribe "Process.kill" do
    ProcessSpecs.use_system_ruby(self)

    before :each do
      @sp1 = ProcessSpecs::Signalizer.new
      @sp2 = ProcessSpecs::Signalizer.new
    end

    after :each do
      @sp1.cleanup
      @sp2.cleanup
    end

    it "signals multiple processes" do
      Process.kill("SIGTERM", @sp1.pid, @sp2.pid)
      @sp1.result.should == "signaled"
      @sp2.result.should == "signaled"
    end

    it "returns the number of processes signaled" do
      Process.kill("SIGTERM", @sp1.pid, @sp2.pid).should == 2
    end
  end

  xdescribe "Process.kill" do
    after :each do
      @sp.cleanup if @sp
    end

    it "signals the process group if the PID is zero" do
      @sp = ProcessSpecs::Signalizer.new "self"
      @sp.result.should == "signaled"
    end

    it "signals the process group if the signal number is negative" do
      @sp = ProcessSpecs::Signalizer.new "group_numeric"
      @sp.result.should == "signaled"
    end

    it "signals the process group if the short signal name starts with a minus sign" do
      @sp = ProcessSpecs::Signalizer.new "group_short_string"
      @sp.result.should == "signaled"
    end

    it "signals the process group if the full signal name starts with a minus sign" do
      @sp = ProcessSpecs::Signalizer.new "group_full_string"
      @sp.result.should == "signaled"
    end
  end
end
