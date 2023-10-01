require_relative '../../spec_helper'
require_relative 'shared/scheduler'

describe 'Fiber.scheduler with Kernel.sleep' do
  after :each do
    Fiber.set_scheduler(nil)
  end

  it 'can interleave fibers with Kernel.sleep' do
    scheduler = Scheduler.new
    Fiber.set_scheduler(scheduler)
    events = []

    sleeper = Fiber.new do
      events << 'Going to sleep'
      sleep(0.01)
      events << 'Woken up'
    end

    barista = Fiber.new do
      events << 'Coffee'
    end

    sleeper.resume
    barista.resume

    scheduler.close

    events.should == ['Going to sleep', 'Coffee', 'Woken up']
  end

  it 'does not interleave blocking fibers' do
    scheduler = Scheduler.new
    Fiber.set_scheduler(scheduler)
    events = []

    sleeper = Fiber.new(blocking: true) do
      events << 'Going to sleep'
      sleep(0.01)
      events << 'Woken up'
    end

    barista = Fiber.new do
      events << 'Coffee'
    end

    sleeper.resume
    barista.resume

    scheduler.close

    events.should == ['Going to sleep', 'Woken up', 'Coffee']
  end

  it 'does not interleave fibers without scheduler' do
    events = []

    sleeper = Fiber.new do
      events << 'Going to sleep'
      sleep(0.01)
      events << 'Woken up'
    end

    barista = Fiber.new do
      events << 'Coffee'
    end

    sleeper.resume
    barista.resume

    events.should == ['Going to sleep', 'Woken up', 'Coffee']
  end
end

