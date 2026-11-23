require 'minitest/autorun'
require_relative '../support/nat_binary'
require_relative '../support/command'

class QueueInterruptTest < Minitest::Test
  def test_nonblocking_pop_defers_interrupts_while_waiting_for_the_internal_lock
    code = <<~RUBY
      queue = Queue.new
      mutex = queue.instance_variable_get(:@mutex)
      ready = Queue.new
      result = []
      mutex.lock
      worker = Thread.new do
        begin
          Thread.handle_interrupt(RuntimeError => :on_blocking) do
            begin
              ready << true
              queue.pop(true)
            rescue ThreadError
            rescue RuntimeError
              result << :interrupted
            end
          end
        rescue RuntimeError
          result << :deferred
        end
      end
      ready.pop
      worker.raise 'deferred interrupt'
      sleep 1
      p worker.alive?
      mutex.unlock
      worker.join
      p result
    RUBY
    output = Command.new(NAT_BINARY, '-e', code, timeout: 120).run
    assert_equal ['true', '[:deferred]'], output.lines.map(&:strip).reject(&:empty?)
  end
end
