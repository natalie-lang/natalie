require_relative '../../spec_helper'

NATFIXME 'Add a stub fiber.rb', exception: LoadError do
  require 'fiber'
end

describe "Fiber.current" do
  ruby_version_is "3.1" do
    it "is available without an extra require" do
      ruby_exe("print Fiber.current.class", options: '--disable-gems --disable-did-you-mean').should == "Fiber"
    end
  end

  it "returns the root Fiber when called outside of a Fiber" do
    root = Fiber.current
    root.should be_an_instance_of(Fiber)
    # We can always transfer to the root Fiber; it will never die
    5.times do
      NATFIXME 'Implement Fiber#transfer', exception: NoMethodError, message: "undefined method `transfer'" do
        root.transfer.should be_nil
      end
      NATFIXME 'Implement Fiber#alive?', exception: NoMethodError, message: "undefined method `alive?'" do
        root.alive?.should be_true
      end
    end
  end

  it "returns the current Fiber when called from a Fiber" do
    fiber = Fiber.new do
      this = Fiber.current
      this.should be_an_instance_of(Fiber)
      this.should == fiber
      NATFIXME 'Implement Fiber#alive?', exception: NoMethodError, message: "undefined method `alive?'" do
        this.alive?.should be_true
      end
    end
    fiber.resume
  end

  it "returns the current Fiber when called from a Fiber that transferred to another" do
    NATFIXME 'Implement Fiber#transfer', exception: NoMethodError, message: "undefined method `transfer'" do
      states = []
      fiber = Fiber.new do
        states << :fiber
        this = Fiber.current
        this.should be_an_instance_of(Fiber)
        this.should == fiber
        this.alive?.should be_true
      end

      fiber2 = Fiber.new do
        states << :fiber2
        fiber.transfer
        flunk
      end

      fiber3 = Fiber.new do
        states << :fiber3
        fiber2.transfer
        states << :fiber3_terminated
      end

      fiber3.resume

      states.should == [:fiber3, :fiber2, :fiber, :fiber3_terminated]
    end
  end
end
