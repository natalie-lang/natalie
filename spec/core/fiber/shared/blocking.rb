describe :non_blocking_fiber, shared: true do
  context "root Fiber of the main thread" do
    it "returns false" do
      NATFIXME 'Implement Fiber.blocking? and Fiber#blocking?', exception: NoMethodError, message: "undefined method `blocking?'" do
        fiber = Fiber.new { @method.call }
        blocking = fiber.resume

        blocking.should == false
      end
    end

    it "returns false for blocking: false" do
      NATFIXME 'Implement Fiber.blocking? and Fiber#blocking?', exception: NoMethodError, message: "undefined method `blocking?'" do
        fiber = Fiber.new(blocking: false) { @method.call }
        blocking = fiber.resume

        blocking.should == false
      end
    end
  end

  context "root Fiber of a new thread" do
    it "returns false" do
      NATFIXME 'Threads', exception: NameError, message: 'uninitialized constant Thread' do
        thread = Thread.new do
          fiber = Fiber.new { @method.call }
          blocking = fiber.resume

          blocking.should == false
        end

        thread.join
      end
    end

    it "returns false for blocking: false" do
      NATFIXME 'Threads', exception: NameError, message: 'uninitialized constant Thread' do
        thread = Thread.new do
          fiber = Fiber.new(blocking: false) { @method.call }
          blocking = fiber.resume

          blocking.should == false
        end

        thread.join
      end
    end
  end
end
