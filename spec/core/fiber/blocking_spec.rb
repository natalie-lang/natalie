require_relative '../../spec_helper'
require_relative 'shared/blocking'

NATFIXME 'Add a stub fiber.rb', exception: LoadError do
  require "fiber"
end

describe "Fiber.blocking?" do
  # NATFIXME: Shared with Fiber#blocking?
  # it_behaves_like :non_blocking_fiber, -> { Fiber.blocking? }

  context "when fiber is blocking" do
    context "root Fiber of the main thread" do
      it "returns 1 for blocking: true" do
        NATFIXME 'Implement Fiber.blocking?', exception: NoMethodError, message: "undefined method `blocking?' for Fiber:Class" do
          fiber = Fiber.new(blocking: true) { Fiber.blocking? }
          blocking = fiber.resume

          blocking.should == 1
        end
      end
    end

    context "root Fiber of a new thread" do
      it "returns 1 for blocking: true" do
        NATFIXME 'Threads', exception: NameError, message: 'uninitialized constant Thread' do
          thread = Thread.new do
            fiber = Fiber.new(blocking: true) { Fiber.blocking? }
            blocking = fiber.resume

            blocking.should == 1
          end

          thread.join
        end
      end
    end
  end
end

describe "Fiber#blocking?" do
  it_behaves_like :non_blocking_fiber, -> { Fiber.current.blocking? }

  context "when fiber is blocking" do
    context "root Fiber of the main thread" do
      it "returns true for blocking: true" do
        fiber = Fiber.new(blocking: true) { Fiber.current.blocking? }
        blocking = fiber.resume

        blocking.should == true
      end
    end

    context "root Fiber of a new thread" do
      it "returns true for blocking: true" do
        NATFIXME 'Threads', exception: NameError, message: 'uninitialized constant Thread' do
          thread = Thread.new do
            fiber = Fiber.new(blocking: true) { Fiber.current.blocking? }
            blocking = fiber.resume

            blocking.should == true
          end

          thread.join
        end
      end
    end
  end
end

ruby_version_is "3.2" do
  describe "Fiber.blocking" do
    context "when fiber is non-blocking" do
      it "can become blocking" do
        fiber = Fiber.new(blocking: false) do
          Fiber.blocking do |f|
            f.blocking? ? :blocking : :non_blocking
          end
        end

        blocking = fiber.resume
        blocking.should == :blocking
      end
    end
  end
end
