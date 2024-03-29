require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe "Thread#alive?" do
  it "can check it's own status" do
    ThreadSpecs.status_of_current_thread.should.alive?
  end

  it "describes a running thread" do
    ThreadSpecs.status_of_running_thread.should.alive?
  end

  it "describes a sleeping thread" do
    ThreadSpecs.status_of_sleeping_thread.should.alive?
  end

  it "describes a blocked thread" do
    ThreadSpecs.status_of_blocked_thread.should.alive?
  end

  it "describes a completed thread" do
    ThreadSpecs.status_of_completed_thread.should_not.alive?
  end

  it "describes a killed thread" do
    ThreadSpecs.status_of_killed_thread.should_not.alive?
  end

  # NATFIXME: The below specs are all dying and we haven't implemented
  # Thread.abort_on_exception nor Thread.report_on_exception yet.

  #it "describes a thread with an uncaught exception" do
    #ThreadSpecs.status_of_thread_with_uncaught_exception.should_not.alive?
  #end

  #it "describes a dying running thread" do
    #ThreadSpecs.status_of_dying_running_thread.should.alive?
  #end

  #it "describes a dying sleeping thread" do
    #ThreadSpecs.status_of_dying_sleeping_thread.should.alive?
  #end

  # NATFIXME: Fix ensure in dying thread.
  #it "returns true for a killed but still running thread" do
    #exit = false
    #t = Thread.new do
      #begin
        #sleep
      #ensure
        #Thread.pass until exit
      #end
    #end

    #ThreadSpecs.spin_until_sleeping(t)

    #t.kill
    #t.should.alive?
    #exit = true
    #t.join
  #end
end
