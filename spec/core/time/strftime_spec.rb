# encoding: utf-8

require_relative '../../spec_helper'
require_relative '../../shared/time/strftime_for_date'
require_relative '../../shared/time/strftime_for_time'

describe "Time#strftime" do
  before :all do
    @new_date = -> y, m, d { Time.gm(y,m,d) }
    @new_time = -> *args { Time.gm(*args) }
    @new_time_in_zone = -> zone, offset, *args {
      with_timezone(zone, offset) do
        Time.new(*args)
      end
    }
    @new_time_with_offset = -> y, m, d, h, min, s, offset {
      Time.new(y,m,d,h,min,s,offset)
    }

    @time = @new_time[2001, 2, 3, 4, 5, 6]
  end

  it_behaves_like :strftime_date, :strftime
  it_behaves_like :strftime_time, :strftime

  # Differences with date
  it "requires an argument" do
    -> { @time.strftime }.should raise_error(ArgumentError)
  end

  # %Z is zone name or empty for Time
  it "should be able to show the timezone if available" do
    NATFIXME 'it should be able to show the timezone if available', exception: SpecFailedException do
      @time.strftime("%Z").should == @time.zone
      with_timezone("UTC", 0) do
        Time.gm(2000).strftime("%Z").should == "UTC"
      end

      Time.new(2000, 1, 1, 0, 0, 0, 42).strftime("%Z").should == ""
    end
  end

  # %v is %e-%^b-%Y for Time
  it "should be able to show the commercial week" do
    NATFIXME 'it should be able to show the commercial week', exception: SpecFailedException do
      @time.strftime("%v").should == " 3-FEB-2001"
      @time.strftime("%v").should == @time.strftime('%e-%^b-%Y')
    end
  end

  # Date/DateTime round at creation time, but Time does it in strftime.
  it "rounds an offset to the nearest second when formatting with %z" do
    time = @new_time_with_offset[2012, 1, 1, 0, 0, 0, Rational(36645, 10)]
    NATFIXME 'it rounds an offset to the nearest second when formatting with %z', exception: SpecFailedException do
      time.strftime("%::z").should == "+01:01:05"
    end
  end

  it "supports RFC 3339 UTC for unknown offset local time, -0000, as %-z" do
    time = Time.gm(2022)

    time.strftime("%z").should == "+0000"
    NATFIXME 'it supports RFC 3339 UTC for unknown offset local time, -0000, as %-z', exception: SpecFailedException do
      time.strftime("%-z").should == "-0000"
      time.strftime("%-:z").should == "-00:00"
      time.strftime("%-::z").should == "-00:00:00"
    end
  end

  it "applies '-' flag to UTC time" do
    time = Time.utc(2022)
    NATFIXME "it applies '-' flag to UTC time", exception: SpecFailedException do
      time.strftime("%-z").should == "-0000"
    end

    time = Time.gm(2022)
    NATFIXME "it applies '-' flag to UTC time", exception: SpecFailedException do
      time.strftime("%-z").should == "-0000"
    end

    time = Time.new(2022, 1, 1, 0, 0, 0, "Z")
    NATFIXME "it applies '-' flag to UTC time", exception: SpecFailedException do
      time.strftime("%-z").should == "-0000"
    end

    time = Time.new(2022, 1, 1, 0, 0, 0, "-00:00")
    NATFIXME "it applies '-' flag to UTC time", exception: SpecFailedException do
      time.strftime("%-z").should == "-0000"
    end

    NATFIXME 'Implement Time#utc', exception: NoMethodError, message: "undefined method 'utc' for an instance of Time" do
      time = Time.new(2022, 1, 1, 0, 0, 0, "+03:00").utc
      time.strftime("%-z").should == "-0000"
    end
  end

  it "ignores '-' flag for non-UTC time" do
    time = Time.new(2022, 1, 1, 0, 0, 0, "+03:00")
    NATFIXME "it ignores '-' flag for non-UTC time", exception: SpecFailedException do
      time.strftime("%-z").should == "+0300"
    end
  end

  it "works correctly with width, _ and 0 flags, and :" do
    NATFIXME 'Implement Time#utc', exception: NoMethodError, message: "undefined method 'utc' for an instance of Time" do
      Time.now.utc.strftime("%-_10z").should == "      -000"
      Time.now.utc.strftime("%-10z").should == "-000000000"
      Time.now.utc.strftime("%-010:z").should == "-000000:00"
      Time.now.utc.strftime("%-_10:z").should == "     -0:00"
      Time.now.utc.strftime("%-_10::z").should == "  -0:00:00"
    end
  end
end
