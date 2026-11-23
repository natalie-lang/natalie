require 'minitest/autorun'
require_relative '../support/nat_binary'
require_relative '../support/command'

class ThreadRaiseKillTest < Minitest::Test
  def test_kill_preserves_an_exception_already_removed_from_the_pending_queue
    code = <<~RUBY
      main = Thread.current
      normalizing = Queue.new
      error_class = Class.new(RuntimeError) do
        define_method(:exception) do |*args|
          if Thread.current != main
            normalizing << true
            loop { Thread.pass }
          end
          self
        end
      end
      worker = Thread.new do
        Thread.current.report_on_exception = false
        sleep
      end
      Thread.pass until worker.stop?
      worker.raise error_class.new('in-flight exception')
      normalizing.pop
      worker.kill
      begin
        worker.join
      rescue => error
        puts error.message
      end
    RUBY
    output = Command.new(NAT_BINARY, '-e', code, timeout: 120).run
    assert_equal 'in-flight exception', output.strip
  end
end
