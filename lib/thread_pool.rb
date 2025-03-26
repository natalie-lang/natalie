class ThreadPool
  def initialize(size)
    @size = size
    @jobs = Thread::Queue.new
    @pool =
      Array.new(@size) do
        Thread.new do
          catch(:exit) do
            loop do
              job, args = @jobs.pop
              job.call(*args)
            end
          end
        end
      end
  end

  def schedule(*args, &block)
    @jobs << [block, args]
  end

  def shutdown
    @size.times { schedule { throw :exit } }
    @pool.map(&:join)
  end
end
