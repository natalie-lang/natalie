# encoding: utf-8

require_relative '../../spec_helper'

describe "Symbol#encoding for ASCII symbols" do
  it "is US-ASCII" do
    NATFIXME 'Implement Symbol#encoding', exception: SpecFailedException do
      :foo.encoding.name.should == "US-ASCII"
    end
  end

  it "is US-ASCII after converting to string" do
    NATFIXME 'Automatically encode input as US-ASCII if input is ascii compatible', exception: SpecFailedException do
      :foo.to_s.encoding.name.should == "US-ASCII"
    end
  end
end

describe "Symbol#encoding for UTF-8 symbols" do
  it "is UTF-8" do
    :åäö.encoding.name.should == "UTF-8"
  end

  it "is UTF-8 after converting to string" do
    :åäö.to_s.encoding.name.should == "UTF-8"
  end
end
