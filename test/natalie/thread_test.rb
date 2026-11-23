require_relative '../spec_helper'

$results = []

def create_threads
  1
    .upto(5)
    .map do |i|
      Thread.new do
        x = "#{i}foo"
        $results << x
        sleep 0.1
        $results << x + 'bar'
      end
    end
end

describe 'Thread' do
  before do
    @report_setting = Thread.report_on_exception
    @abort_setting = Thread.abort_on_exception
  end

  after do
    GC.start # Trigger GC bugs here ;-)
    Thread.report_on_exception = @report_setting
    Thread.abort_on_exception = @abort_setting
  end

  it 'works' do
    threads = create_threads
    GC.start
    threads.each(&:join)
    $results.size.should == 10
  end

  it 'keeps interrupted threads alive while GC scans their registers' do
    10.times do
      ready = Queue.new
      threads =
        8.times.map do
          Thread.new do
            ready << true
            sleep
          end
        end
      threads.size.times { ready.pop }
      threads.each(&:kill)
      GC.start
      threads.each(&:join)
    end
  end

  describe '#join' do
    it 'waits for the thread to finish' do
      start = Time.now
      t = Thread.new { sleep 0.1 }
      t.join
      (Time.now - start).should >= 0.1
    end

    it 'returns the thread' do
      t = Thread.new { 1 }
      t.join.should == t
    end

    it 'can be called multiple times' do
      t = Thread.new { 1 }
      t.join.should == t

      # make sure thread id reuse doesn't cause later join to block
      other_threads = 1.upto(10).map { Thread.new { sleep } }
      sleep 0.1

      # if the thread id gets reused and we are using pthread_join with that id,
      # then this will block on one of the above threads.
      10.times { t.join.should == t }

      other_threads.each(&:kill)
    end
  end

  describe '#value' do
    it 'returns its value' do
      t = Thread.new { 101 }
      t.join.should == t
      t.value.should == 101
    end

    it 'calls join implicitly' do
      t =
        Thread.new do
          sleep 0.1
          102
        end
      t.value.should == 102
    end
  end

  describe 'Fibers within threads' do
    it 'works' do
      t =
        Thread.new do
          @f = Fiber.new { 1 }
          @f.resume.should == 1
        end
      t.join
    end
  end

  describe '.list' do
    it 'keeps a list of all threads' do
      Thread.list.should == [Thread.current]
      t = Thread.new { sleep 0.1 }
      Thread.list.should == [Thread.current, t]
      t.join
      Thread.list.should == [Thread.current]
    end
  end

  describe 'abort_on_exception' do
    it 'raises an error in the main thread if either Thread.abort_on_exception or Thread#abort_on_exception is true' do
      [[false, false], [true, false], [false, true], [true, true]].each do |global, local|
        Thread.abort_on_exception = global

        t =
          Thread.new do
            Thread.current.report_on_exception = false
            Thread.pass until Thread.main.stop?
            Thread.current.abort_on_exception = local
            raise 'foo'
          end

        # this is always false by default
        t.abort_on_exception.should == false

        if global || local
          begin
            sleep
          rescue StandardError => e
            e.message.should == 'foo'
          end
        else
          sleep 0.1
          # should not raise
        end
      end
    end
  end

  describe 'Thread#report_on_exception' do
    it 'defaults to Thread.report_on_exception' do
      Thread.report_on_exception = false
      t1 = Thread.new {}
      t1.report_on_exception.should == false

      Thread.report_on_exception = true
      t2 = Thread.new {}
      t2.report_on_exception.should == true

      t1.report_on_exception.should == false
    end
  end

  describe '#kill' do
    describe 'killing a thread with no blocking IO/system calls' do
      # NATFIXME: Need a way to interrupt a non-blocking thread.
      xit 'works' do
        running = false
        ensure_ran = false
        t =
          Thread.new do
            running = true
            loop do
              # noop
            end
          ensure
            ensure_ran = true
          end
        Thread.pass until running
        t.kill
        t.join
        t.status.should == false
        ensure_ran.should == true
      end
    end
  end

  describe '#raise' do
    # Two raises queued under :never must both arrive in FIFO order as the
    # worker hits successive checkpoints. Pre-refactor, the second raise
    # overwrote the first (single-slot delivery channel), so only one was
    # ever observed.
    it 'queues multiple async raises in FIFO order' do
      seen = []
      ready = Queue.new
      proceed = Queue.new

      t =
        Thread.new do
          Thread.current.report_on_exception = false
          begin
            Thread.handle_interrupt(RuntimeError => :never) do
              ready << true
              proceed.pop
            end
          rescue RuntimeError => e
            seen << e.message
          end

          # The second queued raise drains at the next blocking checkpoint.
          # If we didn't run another checkpoint here, leftover queue items
          # would be silently dropped at thread exit.
          begin
            sleep 0
          rescue RuntimeError => e
            seen << e.message
          end
        end

      ready.pop
      t.raise RuntimeError, 'first'
      t.raise RuntimeError, 'second'
      proceed << true
      t.join

      seen.should == %w[first second]
    end
  end

  describe '.handle_interrupt' do
    it 'raises ArgumentError when called without a block' do
      -> { Thread.handle_interrupt(RuntimeError => :immediate) }.should raise_error(ArgumentError)
    end

    it 'coerces the mask via to_hash' do
      coerced =
        Class
          .new do
            def to_hash
              { RuntimeError => :immediate }
            end
          end
          .new

      ran = false
      caught = nil
      t =
        Thread.new do
          Thread.current.report_on_exception = false
          begin
            Thread.handle_interrupt(coerced) do
              sleep 0.5
              ran = true
            end
          rescue => e
            caught = e.message
          end
        end
      sleep 0.05
      t.raise RuntimeError, 'x'
      t.join

      ran.should == false
      caught.should == 'x'
    end

    it 'silently ignores non-Module keys in the mask' do
      ran = false
      t =
        Thread.new do
          Thread.current.report_on_exception = false
          # MRI permits any key here; the lookup just never matches and
          # falls through to the default :immediate timing.
          Thread.handle_interrupt('not_a_class' => :never) { ran = true }
        end
      t.join

      ran.should == true
    end

    it 'raises ArgumentError for an unknown timing symbol' do
      -> { Thread.handle_interrupt(RuntimeError => :sometime) {} }.should raise_error(ArgumentError)
    end
  end

  describe '#kill' do
    # Race: kill is set while the worker is in Mutex#lock spin; main then
    # releases the mutex; the worker's next try_lock succeeds and the spin
    # exits without re-running the deliver_pending checkpoint, so the
    # thread completes normally past the kill. The undelivered kill must
    # not propagate to join (MRI behaviour: an undelivered kill is dropped).
    it 'does not leak ThreadKillError to the joiner when the kill never fires' do
      m = Mutex.new
      m.lock

      th =
        Thread.new do
          Thread.current.report_on_exception = false
          m.synchronize { :ok }
        end

      Thread.pass while th.status == 'run' && !th.stop?
      th.kill
      m.unlock

      -> { th.join }.should_not raise_error
    end

    # When a single mask names both a parent class and a more specific
    # subclass, the timing of the most-specific match in the exception's
    # ancestor chain wins -- regardless of hash insertion order.
    it 'prefers the most-specific class match within a mask' do
      ran = false
      caught = nil

      t =
        Thread.new do
          Thread.current.report_on_exception = false
          begin
            # StandardError appears first in the hash but RuntimeError is a
            # more-specific match for the raised exception, so :immediate
            # wins -- the sleep should be interrupted, not deferred.
            Thread.handle_interrupt(StandardError => :never, RuntimeError => :immediate) do
              sleep 0.5
              ran = true
            end
          rescue RuntimeError => e
            caught = e.message
          end
        end

      sleep 0.05
      t.raise RuntimeError, 'specific'
      t.join

      ran.should == false
      caught.should == 'specific'
    end

    # Symmetrically: subclass listed first, raised with the parent. Only the
    # parent entry can match, so we get its timing.
    it 'falls back to a less-specific class when the more-specific one does not match' do
      ran = false
      caught = nil

      t =
        Thread.new do
          Thread.current.report_on_exception = false
          begin
            Thread.handle_interrupt(RuntimeError => :immediate, StandardError => :never) do
              sleep 0.5
              ran = true
            end
          rescue ArgumentError => e
            caught = e.message
          end
        end

      sleep 0.05
      t.raise ArgumentError, 'general'
      t.join

      # ArgumentError is a StandardError but not a RuntimeError, so :never
      # applies and the sleep completes; the deferred raise fires at frame pop.
      ran.should == true
      caught.should == 'general'
    end
  end
end
