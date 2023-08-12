require_relative '../../spec_helper'

describe "Regexp.last_match" do
  it "returns MatchData instance when not passed arguments" do
    /c(.)t/ =~ 'cat'

    Regexp.last_match.should be_kind_of(MatchData)
  end

  it "returns the nth field in this MatchData when passed an Integer" do
    /c(.)t/ =~ 'cat'
    Regexp.last_match(1).should == 'a'
  end

  it "returns nil when there is no match" do
    /foo/ =~ "TEST123"
    Regexp.last_match(:test).should == nil
    Regexp.last_match(1).should == nil
    Regexp.last_match(Object.new).should == nil
    Regexp.last_match("test").should == nil
  end

  describe "when given a Symbol" do
    it "returns a named capture" do
      /(?<test>[A-Z]+.*)/ =~ "TEST123"
      Regexp.last_match(:test).should == "TEST123"
    end

    it "raises an IndexError when given a missing name" do
      /(?<test>[A-Z]+.*)/ =~ "TEST123"
      NATFIXME "this error gets thrown, but when in a proc it doesn't get bubbled up properly", exception: SpecFailedException do
        -> { Regexp.last_match(:missing) }.should raise_error(IndexError)
      end

      # NATFIXME: Alternate implementation of the test
      error = begin
        Regexp.last_match(:missing)
        nil
      rescue IndexError => e
        e
      end
      error.should be_kind_of(IndexError)
    end
  end

  describe "when given a String" do
    it "returns a named capture" do
      /(?<test>[A-Z]+.*)/ =~ "TEST123"
      Regexp.last_match("test").should == "TEST123"
    end
  end

  describe "when given an Object" do
    it "coerces argument to an index using #to_int" do
      obj = mock("converted to int")
      obj.should_receive(:to_int).and_return(1)
      /(?<test>[A-Z]+.*)/ =~ "TEST123"
      Regexp.last_match(obj).should == "TEST123"
    end

    it "raises a TypeError when unable to coerce" do
      obj = Object.new
      /(?<test>[A-Z]+.*)/ =~ "TEST123"
      NATFIXME "this error gets thrown, but when in a proc it doesn't get bubbled up properly", exception: SpecFailedException do
        -> { Regexp.last_match(obj) }.should raise_error(TypeError)
      end

      # NATFIXME: Alternate implementation of the test
      error = begin
        Regexp.last_match(obj)
        nil
      rescue TypeError => e
        e
      end
      error.should be_kind_of(TypeError)
    end
  end
end
