describe :queue_num_waiting, shared: true do
  it "reports the number of threads waiting on the queue" do
    q = @object.call
    threads = []

    NATFIXME 'Implement Queue#num_waiting', exception: NoMethodError, message: "undefined method `num_waiting' for an instance of Queue" do
      5.times do |i|
        q.num_waiting.should == i
        t = Thread.new { q.deq }
        Thread.pass until q.num_waiting == i+1
        threads << t
      end

      threads.each { q.enq Object.new }
      threads.each {|t| t.join }
    end
  end
end
