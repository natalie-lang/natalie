require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe "Thread.stop" do
  it "causes the current thread to sleep indefinitely" do
    t = Thread.new { Thread.stop; 5 }
    Thread.pass while t.status and t.status != 'sleep'
    t.status.should == 'sleep'
    t.run
    t.value.should == 5
  end
end

describe "Thread#stop?" do
  it "can check it's own status" do
    ThreadSpecs.status_of_current_thread.should_not.stop?
  end

  it "describes a running thread" do
    ThreadSpecs.status_of_running_thread.should_not.stop?
  end

  it "describes a sleeping thread" do
    ThreadSpecs.status_of_sleeping_thread.should.stop?
  end

  it "describes a blocked thread" do
    ThreadSpecs.status_of_blocked_thread.should.stop?
  end

  it "describes a completed thread" do
    ThreadSpecs.status_of_completed_thread.should.stop?
  end

  it "describes a killed thread" do
    ThreadSpecs.status_of_killed_thread.should.stop?
  end

  # NATFIXME: The below specs are all dying and we haven't implemented
  # Thread.abort_on_exception nor Thread.report_on_exception yet.

  #it "describes a thread with an uncaught exception" do
    #ThreadSpecs.status_of_thread_with_uncaught_exception.should.stop?
  #end

  #it "describes a dying running thread" do
    #ThreadSpecs.status_of_dying_running_thread.should_not.stop?
  #end

  #it "describes a dying sleeping thread" do
    #ThreadSpecs.status_of_dying_sleeping_thread.should.stop?
  #end

  #it "describes a dying thread after sleep" do
    #ThreadSpecs.status_of_dying_thread_after_sleep.should_not.stop?
  #end
end
