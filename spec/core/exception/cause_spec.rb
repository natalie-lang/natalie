require_relative '../../spec_helper'

describe "Exception#cause" do
  it "returns the active exception when an exception is raised" do
    begin
      raise Exception, "the cause"
    rescue Exception
      begin
        raise RuntimeError, "the consequence"
      rescue RuntimeError => e
        e.should be_an_instance_of(RuntimeError)
        e.message.should == "the consequence"

        NATFIXME 'Save current exception as cause', exception: SpecFailedException do
          e.cause.should be_an_instance_of(Exception)
          e.cause.message.should == "the cause"
        end
      end
    end
  end

  it "is set for user errors caused by internal errors" do
    -> {
      begin
        1 / 0
      rescue
        raise "foo"
      end
    }.should raise_error(RuntimeError) { |e|
      NATFIXME 'Save current exception as cause', exception: SpecFailedException do
        e.cause.should be_kind_of(ZeroDivisionError)
      end
    }
  end

  it "is set for internal errors caused by user errors" do
    cause = RuntimeError.new "cause"
    -> {
      begin
        raise cause
      rescue
        1 / 0
      end
    }.should raise_error(ZeroDivisionError) { |e|
      NATFIXME 'Save current exception as cause', exception: SpecFailedException do
        e.cause.should equal(cause)
      end
    }
  end

  it "is not set to the exception itself when it is re-raised" do
    -> {
      begin
        raise RuntimeError
      rescue RuntimeError => e
        raise e
      end
    }.should raise_error(RuntimeError) { |e|
      e.cause.should == nil
    }
  end
end
