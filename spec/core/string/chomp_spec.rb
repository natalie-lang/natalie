require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe "String#chomp" do
  it "doesn't mutate the calling object" do
    str = "hello\n"
    str.chomp.should == "hello"
    str.should == "hello\n"
  end

  it "when passed no arguments, removes last trailing \\n, \\r, and \\r\\n" do
    "hello".chomp.should == "hello"
    "hello\n".chomp.should == "hello"
    "hello\r\n".chomp.should == "hello"
    "hello\n\r".chomp.should == "hello\n"
    "hello\r".chomp.should == "hello"
    "hello \n there".chomp.should == "hello \n there"
    "hello\r\r".chomp.should == "hello\r"
    "hello\r\r\r".chomp.should == "hello\r\r"
    "hello\r\n\r\r\n".chomp.should == "hello\r\n\r"
    "hello\n\n\n".chomp.should == "hello\n\n"
    "hello\r\n\r\n".chomp.should == "hello\r\n"
  end

  it "when passed empty string, removes all trailing \\n and \\r\\n but not \\r" do
    "hello\r\n\r\n".chomp('').should == "hello"
    "hello\r\n\r\r\n".chomp('').should == "hello\r\n\r"
    "hello\r\r".chomp('').should == "hello\r\r"
    "hello\r\r\r".chomp('').should == "hello\r\r\r"
    "hello\r\r\n".chomp.should == "hello\r"
    "hello\r\r\n".chomp('').should == "hello\r"
    "hello\n\n\n".chomp('').should == "hello"
  end 

  it "works with non-empty string arguments" do
    "hello".chomp("llo").should == "he"
    "natalie".chomp("natalie").should == ""
    "natalie".chomp("atalie").should == "n"
    "natalie natalie".chomp("natalie").should == "natalie "
    "natalie \nnatalie".chomp("natalie").should == "natalie \n"
    "natalie natalie\r".chomp("natalie").should == "natalie natalie\r"
    "natalie natalie\n".chomp("natalie").should == "natalie natalie\n"
    "hello world".chomp("hello").should == "hello world"
    "hello worldworld".chomp("world").should == "hello world"
    "hello".chomp("allohello").should == "hello"
  end

  it "does not mutate its argument" do
    str = String.new("llo")
    "hello".chomp(str).should == "he"
    str.should == "llo"
  end
end