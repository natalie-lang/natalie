require_relative '../../spec_helper'

describe "Mutex#unlock" do
  it "raises ThreadError unless Mutex is locked" do
    mutex = Mutex.new
    -> { mutex.unlock }.should raise_error(ThreadError)
  end

  it "raises ThreadError unless thread owns Mutex" do
    mutex = Mutex.new
    wait = Mutex.new
    wait.lock
    th = Thread.new do
      mutex.lock
      wait.lock
    end

    # avoid race on mutex.lock
    Thread.pass until mutex.locked?
    Thread.pass until th.stop?
    Thread.pass while th.status == 'run'

    -> { mutex.unlock }.should raise_error(ThreadError)

    wait.unlock
    th.join
  end

  it "raises ThreadError if previously locking thread is gone" do
    mutex = Mutex.new
    th = Thread.new do
      mutex.lock
    end

    th.join

    -> { mutex.unlock }.should raise_error(ThreadError)
  end
end
