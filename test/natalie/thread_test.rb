require_relative '../spec_helper'

$results = []

def create_threads
  1.upto(5).map do |i|
    Thread.new do
      x = "#{i}foo"
      $results << x
      sleep 1
      $results << x + "bar"
    end
  end
end

describe 'Thread' do
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
      t.join.should == t
    end
  end

  describe '#value' do
    it 'returns its value' do
      t = Thread.new { 101 }
      t.join.should == t
      t.value.should == 101
    end

    it 'calls join implicitly' do
      t = Thread.new { sleep 1; 102 }
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
end
