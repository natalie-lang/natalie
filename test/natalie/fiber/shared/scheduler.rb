require 'fiber'

class Scheduler
  def initialize
    @waiting = {}
  end

  def run
    until @waiting.empty?
      obj_id, (fiber, _) = @waiting.find { |_, (fiber, timeout)| fiber.alive? && timeout <= current_time }

      unless obj_id.nil?
        @waiting.delete(obj_id)
        fiber.resume
      end
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

    # NATFIXME: Issues when using Fiber objects as hash key, use object_id for the time being.
    @waiting[Fiber.current.object_id] = [Fiber.current, current_time + duration]
    Fiber.yield
  end

  def current_time
    Process.clock_gettime(Process::CLOCK_MONOTONIC)
  end

  def io_wait(io, events, duration) end
  def block(blocker, timeout = nil) end
  def unblock(blocker, fiber) end
end
