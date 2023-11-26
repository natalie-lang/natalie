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

  describe 'Fibers within threads' do
    it 'works' do
      Thread.new do
        @f = Fiber.new do
          1
        end
        @f.resume.should == 1
      end
    end
  end
end
