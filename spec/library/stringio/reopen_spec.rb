require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe "StringIO#reopen when passed [Object, Integer]" do
  before :each do
    @io = StringIO.new("example")
  end

  it "reopens self with the passed Object in the passed mode" do
    @io.reopen("reopened", IO::RDONLY)
    @io.closed_read?.should be_false
    @io.closed_write?.should be_true
    @io.string.should == "reopened"

    @io.reopen(+"reopened, twice", IO::WRONLY)
    @io.closed_read?.should be_true
    @io.closed_write?.should be_false
    @io.string.should == "reopened, twice"

    @io.reopen(+"reopened, another time", IO::RDWR)
    @io.closed_read?.should be_false
    @io.closed_write?.should be_false
    @io.string.should == "reopened, another time"
  end

  it "tries to convert the passed Object to a String using #to_str" do
    obj = mock("to_str")
    obj.should_receive(:to_str).and_return(+"to_str")
    @io.reopen(obj, IO::RDWR)
    @io.string.should == "to_str"
  end

  it "raises a TypeError when the passed Object can't be converted to a String" do
    NATFIXME "it raises a TypeError when the passed Object can't be converted to a String", exception: SpecFailedException do
      -> { @io.reopen(Object.new, IO::RDWR) }.should raise_error(TypeError)
    end
  end

  it "raises an Errno::EACCES when trying to reopen self with a frozen String in write-mode" do
    NATFIXME 'it raises an Errno::EACCES when trying to reopen self with a frozen String in write-mode', exception: SpecFailedException do
      -> { @io.reopen("burn".freeze, IO::WRONLY) }.should raise_error(Errno::EACCES)
      -> { @io.reopen("burn".freeze, IO::WRONLY | IO::APPEND) }.should raise_error(Errno::EACCES)
    end
  end

  it "raises a FrozenError when trying to reopen self with a frozen String in truncate-mode" do
    -> { @io.reopen("burn".freeze, IO::RDONLY | IO::TRUNC) }.should raise_error(FrozenError)
  end

  it "does not raise IOError when passed a frozen String in read-mode" do
    @io.reopen("burn".freeze, IO::RDONLY)
    @io.string.should == "burn"
  end
end

describe "StringIO#reopen when passed [Object, Object]" do
  before :each do
    @io = StringIO.new("example")
  end

  it "reopens self with the passed Object in the passed mode" do
    @io.reopen("reopened", "r")
    @io.closed_read?.should be_false
    @io.closed_write?.should be_true
    @io.string.should == "reopened"

    @io.reopen(+"reopened, twice", "r+")
    @io.closed_read?.should be_false
    @io.closed_write?.should be_false
    @io.string.should == "reopened, twice"

    @io.reopen(+"reopened, another", "w+")
    @io.closed_read?.should be_false
    @io.closed_write?.should be_false
    NATFIXME 'it should be truncated when using mode "w+"', exception: SpecFailedException do
      @io.string.should == ""
    end

    @io.reopen(+"reopened, another time", "r+")
    @io.closed_read?.should be_false
    @io.closed_write?.should be_false
    @io.string.should == "reopened, another time"
  end

  it "truncates the passed String when opened in truncate mode" do
    @io.reopen(str = +"reopened", "w")
    NATFIXME 'it truncates the passed String when opened in truncate mode', exception: SpecFailedException do
      str.should == ""
    end
  end

  it "tries to convert the passed Object to a String using #to_str" do
    obj = mock("to_str")
    obj.should_receive(:to_str).and_return("to_str")
    @io.reopen(obj, "r")
    @io.string.should == "to_str"
  end

  it "raises a TypeError when the passed Object can't be converted to a String using #to_str" do
    NATFIXME "it raises a TypeError when the passed Object can't be converted to a String using #to_str", exception: SpecFailedException do
      -> { @io.reopen(Object.new, "r") }.should raise_error(TypeError)
    end
  end

  it "resets self's position to 0" do
    @io.read(5)
    @io.reopen(+"reopened")
    @io.pos.should eql(0)
  end

  it "resets self's line number to 0" do
    @io.gets
    @io.reopen(+"reopened")
    @io.lineno.should eql(0)
  end

  it "tries to convert the passed mode Object to an Integer using #to_str" do
    obj = mock("to_str")
    obj.should_receive(:to_str).and_return("r")
    @io.reopen("reopened", obj)
    @io.closed_read?.should be_false
    @io.closed_write?.should be_true
    @io.string.should == "reopened"
  end

  it "raises an Errno::EACCES error when trying to reopen self with a frozen String in write-mode" do
    NATFIXME 'it raises an Errno::EACCES error when trying to reopen self with a frozen String in write-mode', exception: SpecFailedException do
      -> { @io.reopen("burn".freeze, 'w') }.should raise_error(Errno::EACCES)
      -> { @io.reopen("burn".freeze, 'w+') }.should raise_error(Errno::EACCES)
      -> { @io.reopen("burn".freeze, 'a') }.should raise_error(Errno::EACCES)
      -> { @io.reopen("burn".freeze, "r+") }.should raise_error(Errno::EACCES)
    end
  end

  it "does not raise IOError if a frozen string is passed in read mode" do
    @io.reopen("burn".freeze, "r")
    @io.string.should == "burn"
  end
end

describe "StringIO#reopen when passed [String]" do
  before :each do
    @io = StringIO.new("example")
  end

  it "reopens self with the passed String in read-write mode" do
    @io.close

    @io.reopen(+"reopened")

    @io.closed_write?.should be_false
    @io.closed_read?.should be_false

    @io.string.should == "reopened"
  end

  it "resets self's position to 0" do
    @io.read(5)
    @io.reopen(+"reopened")
    @io.pos.should eql(0)
  end

  it "resets self's line number to 0" do
    @io.gets
    @io.reopen(+"reopened")
    @io.lineno.should eql(0)
  end
end

describe "StringIO#reopen when passed [Object]" do
  before :each do
    @io = StringIO.new("example")
  end

  it "raises a TypeError when passed an Object that can't be converted to a StringIO" do
    NATFIXME "it raises a TypeError when passed an Object that can't be converted to a StringIO", exception: SpecFailedException do
      -> { @io.reopen(Object.new) }.should raise_error(TypeError)
    end
  end

  it "does not try to convert the passed Object to a String using #to_str" do
    obj = mock("not to_str")
    NATFIXME 'it does not try to convert the passed Object to a String using #to_str', exception: SpecFailedException do
      obj.should_not_receive(:to_str)
      -> { @io.reopen(obj) }.should raise_error(TypeError)
    end
  end

  it "tries to convert the passed Object to a StringIO using #to_strio" do
    obj = mock("to_strio")
    NATFIXME 'it tries to convert the passed Object to a StringIO using #to_strio', exception: NoMethodError, message: "undefined method 'to_str' for an instance of MockObject" do
      obj.should_receive(:to_strio).and_return(StringIO.new(+"to_strio"))
      @io.reopen(obj)
      @io.string.should == "to_strio"
    end
  end
end

describe "StringIO#reopen when passed no arguments" do
  before :each do
    @io = StringIO.new("example\nsecond line")
  end

  it "resets self's mode to read-write" do
    @io.close
    @io.reopen
    @io.closed_read?.should be_false
    @io.closed_write?.should be_false
  end

  it "resets self's position to 0" do
    @io.read(5)
    @io.reopen
    @io.pos.should eql(0)
  end

  it "resets self's line number to 0" do
    @io.gets
    @io.reopen
    @io.lineno.should eql(0)
  end
end

# NOTE: Some reopen specs disabled due to MRI bugs. See:
# http://rubyforge.org/tracker/index.php?func=detail&aid=13919&group_id=426&atid=1698
# for details.
describe "StringIO#reopen" do
  before :each do
    @io = StringIO.new(+'hello', 'a')
  end

  # TODO: find out if this is really a bug
  it "reopens a stream when given a String argument" do
    @io.reopen(+'goodbye').should == @io
    @io.string.should == 'goodbye'
    @io << 'x'
    @io.string.should == 'xoodbye'
  end

  it "reopens a stream in append mode when flagged as such" do
    @io.reopen(+'goodbye', 'a').should == @io
    @io.string.should == 'goodbye'
    @io << 'x'
    @io.string.should == 'goodbyex'
  end

  it "reopens and truncate when reopened in write mode" do
    @io.reopen(+'goodbye', 'wb').should == @io
    NATFIXME 'it reopens and truncate when reopened in write mode', exception: SpecFailedException do
      @io.string.should == ''
      @io << 'x'
      @io.string.should == 'x'
    end
  end

  it "truncates the given string, not a copy" do
    str = +'goodbye'
    @io.reopen(str, 'w')
    NATFIXME 'it truncates the given string, not a copy', exception: SpecFailedException do
      @io.string.should == ''
      str.should == ''
    end
  end

  it "does not truncate the content even when the StringIO argument is in the truncate mode" do
    orig_io = StringIO.new(+"Original StringIO", IO::RDWR|IO::TRUNC)
    orig_io.write("BLAH") # make sure the content is not empty

    NATFIXME 'support StringIO argument', exception: NoMethodError, message: "undefined method 'to_str' for an instance of StringIO" do
      @io.reopen(orig_io)
      @io.string.should == "BLAH"
    end
  end

end
