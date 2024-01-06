require_relative '../spec_helper'

$results = []

def create_threads
  1.upto(5).map do |i|
    Thread.new do
      x = "#{i}foo"
      $results << x
      sleep 0.1
      $results << x + "bar"
    end
  end
end

describe 'Thread' do
  before do
    @report_setting = Thread.report_on_exception
    @abort_setting = Thread.abort_on_exception
  end

  after do
    Thread.report_on_exception = @report_setting
    Thread.abort_on_exception = @abort_setting
  end

  it 'works' do
    threads = create_threads
    1_000_000.times do
      'trigger gc'
    end
    threads.each(&:join)
    $results.size.should == 10
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
      other_threads = 1.upto(100).map { Thread.new { sleep } }
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
      t = Thread.new { sleep 0.1; 102 }
      t.value.should == 102
    end
  end

  describe 'Fibers within threads' do
    it 'works' do
      t = Thread.new do
        @f = Fiber.new do
          1
        end
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
      [
        [false, false],
        [true, false],
        [false, true],
        [true, true],
      ].each do |global, local|
        Thread.abort_on_exception = global

        t = Thread.new do
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
        t = Thread.new do
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
end
