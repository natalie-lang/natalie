require_relative '../spec_helper'

describe 'Fiber' do
  it 'works' do
    result = []
    f1 =
      Fiber.new do |*args|
        result << args
        result << 123
        result << Fiber.yield(6, 7)
        result << 456
        :end1
      end

    f2 =
      Fiber.new do |*args|
        result << args
        result << 'abc'
        result << Fiber.yield(8, 9)
        result << 'def'
        :end2
      end

    result << f1.resume(1, 2)
    result << f2.resume(3)
    result << f1.resume(4)
    result << f2.resume(5)
    result << 789

    result.should == [[1, 2], 123, [6, 7], [3], 'abc', [8, 9], 4, 456, :end1, 5, 'def', :end2, 789]
  end

  it 'bubbles up errors' do
    f = Fiber.new { raise 'error' }

    -> { f.resume }.should raise_error(StandardError, 'error')

    # fiber is now dead:
    -> { f.resume }.should raise_error(FiberError, /dead fiber called|attempt to resume a terminated fiber/)
  end

  it 'can be resumed from another fiber' do
    f2 = Fiber.new { Fiber.yield 'hi from f2' }

    f = Fiber.new { Fiber.yield f2.resume }

    f.resume.should == 'hi from f2'
  end

  it 'raises an error when attempting to yield from the main fiber' do
    -> { Fiber.yield 'foo' }.should raise_error(
                                      FiberError,
                                      /can't yield from root fiber|attempt to yield on a not resumed fiber/,
                                    )
  end
end

describe 'Scheduler' do
  it 'validates the scheduler for required methods' do
    # We should not be testing for ordering of these error messages
    required_methods = [:block, :unblock, :kernel_sleep, :io_wait]
    required_methods.each do |missing_method|
      scheduler = Object.new
      required_methods.difference([missing_method]).each do |method|
        scheduler.define_singleton_method(method) {}
      end

      -> { Fiber.set_scheduler(scheduler) }.should raise_error(ArgumentError, /Scheduler must implement ##{missing_method}/)
    end
  end

  it 'can set and get the scheduler' do
    required_methods = [:block, :unblock, :kernel_sleep, :io_wait]
    scheduler = Object.new
    required_methods.each do |method|
      scheduler.define_singleton_method(method) {}
    end
    Fiber.set_scheduler(scheduler)
    Fiber.scheduler.should == scheduler
  end

  it 'can remove the scheduler' do
    required_methods = [:block, :unblock, :kernel_sleep, :io_wait]
    scheduler = Object.new
    required_methods.each do |method|
      scheduler.define_singleton_method(method) {}
    end
    Fiber.set_scheduler(scheduler)
    Fiber.set_scheduler(nil)
    Fiber.scheduler.should be_nil
  end

  it 'can assign a nil scheduler multiple times' do
    Fiber.set_scheduler(nil)
    Fiber.set_scheduler(nil)
    Fiber.scheduler.should be_nil
  end
end
