require_relative '../../../spec_helper'

describe :symbol_succ, shared: true do
  it "returns a successor" do
    :abcd.send(@method).should == :abce
    :THX1138.send(@method).should == :THX1139
  end

  it "propagates a 'carry'" do
    NATFIXME "propagates a 'carry'", exception: SpecFailedException do
      :"1999zzz".send(@method).should == :"2000aaa"
      :ZZZ9999.send(@method).should == :AAAA0000
    end
  end

  it "increments non-alphanumeric characters when no alphanumeric characters are present" do
    NATFIXME "increments non-alphanumeric characters when no alphanumeric characters are present", exception: SpecFailedException do
      :"<<koala>>".send(@method).should == :"<<koalb>>"
    end
    :"***".send(@method).should == :"**+"
  end
end
