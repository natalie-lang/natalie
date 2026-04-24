require_relative '../../spec_helper'
require 'timeout'

describe "Timeout.timeout" do
  it "raises Timeout::Error when it times out with no specified error type" do
    NATFIXME 'Timeout.timeout needs Thread.handle_interrupt', exception: SpecFailedException do
      -> {
        Timeout.timeout(1) do
          sleep
        end
      }.should raise_error(Timeout::Error)
    end
  end

  it "raises specified error type when it times out" do
    -> do
      Timeout.timeout(1, StandardError) do
        sleep
      end
    end.should raise_error(StandardError)
  end

  it "raises specified error type with specified message when it times out" do
    NATFIXME 'Timeout.timeout needs Thread.handle_interrupt', exception: SpecFailedException do
      -> do
        Timeout.timeout(1, StandardError, "foobar") do
          sleep
        end
      end.should raise_error(StandardError, "foobar")
    end
  end

  it "raises specified error type with a default message when it times out if message is nil" do
    NATFIXME 'Timeout.timeout needs Thread.handle_interrupt', exception: SpecFailedException do
      -> do
        Timeout.timeout(1, StandardError, nil) do
          sleep
        end
      end.should raise_error(StandardError, "execution expired")
    end
  end

  it "returns back the last value in the block" do
    NATFIXME 'Timeout.timeout needs Thread.handle_interrupt', exception: NoMethodError, message: /handle_interrupt/ do
      Timeout.timeout(1) do
        42
      end.should == 42
    end
  end

  ruby_version_is "3.4" do
    it "raises an ArgumentError when provided with a negative duration" do
      -> {
        Timeout.timeout(-1)
      }.should raise_error(ArgumentError, "Timeout sec must be a non-negative number")
    end
  end
end
