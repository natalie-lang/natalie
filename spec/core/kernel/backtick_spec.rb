require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe "Kernel#`" do
  before :each do
    @original_external = Encoding.default_external
  end

  after :each do
    Encoding.default_external = @original_external
  end

  it "is a private method" do
    Kernel.should have_private_instance_method(:`)
  end

  it "returns the standard output of the executed sub-process" do
    ip = 'world'
    `echo disc #{ip}`.should == "disc world\n"
  end

  # NATFIXME: Issues with capturing stderr stream?
  xit "lets the standard error stream pass through to the inherited stderr" do
    cmd = ruby_cmd('STDERR.print "error stream"')
    -> {
      `#{cmd}`.should == ""
    }.should output_to_fd("error stream", STDERR)
  end

  it "produces a String in the default external encoding" do
    NATFIXME 'it produces a String in the default external encoding', exception: SpecFailedException do
      Encoding.default_external = Encoding::SHIFT_JIS
      `echo disc`.encoding.should equal(Encoding::SHIFT_JIS)
    end
  end

  it "raises an Errno::ENOENT if the command is not executable" do
    NATFIXME 'it raises an Errno::ENOENT if the command is not executable', exception: SpecFailedException do
      -> { `nonexistent_command` }.should raise_error(Errno::ENOENT)
    end
  end

  platform_is_not :windows do
    it "handles invalid UTF-8 bytes in command" do
      `echo "testing\xC2 a non UTF-8 string"`.b.should == "testing\xC2 a non UTF-8 string\n".b
    end

    it "sets $? to the exit status of the executed sub-process" do
      ip = 'world'
      `echo disc #{ip}`
      $?.should be_kind_of(Process::Status)
      NATFIXME 'Implement Process::Status#stopped?', exception: NoMethodError, message: "undefined method `stopped?' for an instance of Process::Status" do
        $?.should_not.stopped?
      end
      $?.should.exited?
      $?.exitstatus.should == 0
      $?.should.success?
      `echo disc #{ip}; exit 99`
      $?.should be_kind_of(Process::Status)
      NATFIXME 'Implement Process::Status#stopped?', exception: NoMethodError, message: "undefined method `stopped?' for an instance of Process::Status" do
        $?.should_not.stopped?
      end
      $?.should.exited?
      $?.exitstatus.should == 99
      $?.should_not.success?
    end
  end

  platform_is :windows do
    it "sets $? to the exit status of the executed sub-process" do
      ip = 'world'
      `echo disc #{ip}`
      $?.should be_kind_of(Process::Status)
      $?.should_not.stopped?
      $?.should.exited?
      $?.exitstatus.should == 0
      $?.should.success?
      `echo disc #{ip}& exit 99`
      $?.should be_kind_of(Process::Status)
      $?.should_not.stopped?
      $?.should.exited?
      $?.exitstatus.should == 99
      $?.should_not.success?
    end
  end
end

describe "Kernel.`" do
  it "tries to convert the given argument to String using #to_str" do
    (obj = mock('echo test')).should_receive(:to_str).and_return("echo test")
    Kernel.`(obj).should == "test\n"  #` fix vim syntax highlighting
  end
end
