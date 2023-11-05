require 'fiber'

class Scheduler
  def initialize
    @waiting = {}
  end

  def run
    until @waiting.empty?
      fiber, = @waiting.find { |fiber, timeout| fiber.alive? && timeout <= current_time }
      next if fiber.nil?

      @waiting.delete(fiber)
      fiber.resume
    end
  end

  def close
    run
  end

  def kernel_sleep(duration = nil)
    # NATFIXME: We don't have any mechanism to stop a fiber with an infinite sleep, so we should not
    # resume that one.
    if duration.nil?
      Fiber.yield
      return
    end

    @waiting[Fiber.current] = current_time + duration
    Fiber.yield
  end

  def current_time
    Process.clock_gettime(Process::CLOCK_MONOTONIC)
  end

  def io_wait(io, events, duration) end
  def block(blocker, timeout = nil) end
  def unblock(blocker, fiber) end
end
