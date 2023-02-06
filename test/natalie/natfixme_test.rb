require_relative '../spec_helper'


describe "NATFIXME" do

  # Reference expectation check, not testing NATFIXME
  it "fails spec outside natfixme" do
    a = 5
    -> {
      a.should == 6
    }.should raise_error(SpecFailedException)
  end

  it "hides a failing block with klass only" do
    NATFIXME "Descriptive message" do
      raise "fake error which will be hidden"
    end
  end
  it "hides a failing block with klass only" do
    NATFIXME "cant load a missing thing", klass: LoadError do
      load "/tmp/xyzzy.eW91dmVfYmVlbl9lYXRlbl9ieV9hX2dydWUK"
    end
  end

  it "hides a failing block with klass and match" do
    s = "567879"
    NATFIXME "Pending String#foo", klass: NoMethodError, match: /method.*foo/ do
      s.foo.should == "foo567879"
    end
  end
  
  it "fails when the block passes" do
    -> {
      NATFIXME "Pending String#sub" do
        s = "567879".sub(/9/,"9")
        s.should == "567879"
      end
    }.should raise_error(NatalieFixMeException)
  end

  it "fails when the block fails but with the wrong klass" do
    -> {
    NATFIXME "cant load a missing thing", klass: ZeroDivisionError do
      load "/tmp/xyzzy.eW91dmVfYmVlbl9lYXRlbl9ieV9hX2dydWUK"
    end
    }.should raise_error(NatalieFixMeException)
  end

  it "fails when the block fails but with the wrong message" do
    -> {
    NATFIXME "cant load a missing thing", klass: ZeroDivisionError, match: "divided by ZERO" do
      1/0
    end
    }.should raise_error(NatalieFixMeException)
  end

end
