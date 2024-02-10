require_relative '../../spec_helper'
require 'English'

describe "English" do
  it "aliases $! to $ERROR_INFO and $ERROR_INFO still returns an Exception with a backtrace" do
    NATFIXME '$! is rewritten at compile time, impossible to alias', exception: SpecFailedException do
      exception = (1 / 0 rescue $ERROR_INFO)
      exception.should be_kind_of(Exception)
      exception.backtrace.should be_kind_of(Array)
    end
  end

  it "aliases $@ to $ERROR_POSITION and $ERROR_POSITION still returns a backtrace" do
    NATFIXME '$@ is rewritten at compile time, impossible to alias', exception: SpecFailedException do
      (1 / 0 rescue $ERROR_POSITION).should be_kind_of(Array)
    end
  end
end
