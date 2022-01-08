require_relative '../../spec_helper'

describe "TrueClass#to_s" do
  it "returns the string 'true'" do
    true.to_s.should == "true"
  end

  ruby_version_is "2.7" do
    it "returns a frozen string" do
      true.to_s.should.frozen?
    end

    # NATFIXME: TrueClass#to_s should always return the same string
    xit "always returns the same string" do
      true.to_s.should equal(true.to_s)
    end
  end
end
