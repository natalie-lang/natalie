require_relative '../fixtures/classes'

describe :io_binwrite, shared: true do
  before :each do
    @filename = tmp("IO_binwrite_file") + $$.to_s
    File.open(@filename, "w") do |file|
      file << "012345678901234567890123456789"
    end
  end

  after :each do
    rm_r @filename
  end

  it "coerces the argument to a string using to_s" do
    (obj = mock('test')).should_receive(:to_s).and_return('a string')
    IO.send(@method, @filename, obj)
  end

  it "returns the number of bytes written" do
    IO.send(@method, @filename, "abcde").should == 5
  end

  it "accepts options as a keyword argument" do
    if @method == :write
      NATFIXME 'Keyword arguments', exception: ArgumentError, message: 'wrong number of arguments (given 4, expected 2)' do
        IO.send(@method, @filename, "hi", 0, flags: File::CREAT).should == 2

        -> {
          IO.send(@method, @filename, "hi", 0, {flags: File::CREAT})
        }.should raise_error(ArgumentError, "wrong number of arguments (given 4, expected 2..3)")
      end
    else
      IO.send(@method, @filename, "hi", 0, flags: File::CREAT).should == 2

      -> {
        IO.send(@method, @filename, "hi", 0, {flags: File::CREAT})
      }.should raise_error(ArgumentError, "wrong number of arguments (given 4, expected 2..3)")
    end
  end

  it "creates a file if missing" do
    fn = @filename + "xxx"
    begin
      File.should_not.exist?(fn)
      IO.send(@method, fn, "test")
      File.should.exist?(fn)
    ensure
      rm_r fn
    end
  end

  it "creates file if missing even if offset given" do
    fn = @filename + "xxx"
    begin
      File.should_not.exist?(fn)
      if @method == :write
        NATFIXME "Add offset to IO.#{@method}", exception: ArgumentError, message: 'wrong number of arguments (given 3, expected 2)' do
          IO.send(@method, fn, "test", 0)
          File.should.exist?(fn)
        end
      else
        IO.send(@method, fn, "test", 0)
        File.should.exist?(fn)
      end
    ensure
      rm_r fn
    end
  end

  it "truncates the file and writes the given string" do
    IO.send(@method, @filename, "hello, world!")
    File.read(@filename).should == "hello, world!"
  end

  it "doesn't truncate the file and writes the given string if an offset is given" do
    if @method == :write
      NATFIXME "Add offset to IO.#{@method}", exception: ArgumentError, message: 'wrong number of arguments (given 3, expected 2)' do
        IO.send(@method, @filename, "hello, world!", 0)
        File.read(@filename).should == "hello, world!34567890123456789"
        IO.send(@method, @filename, "hello, world!", 20)
        File.read(@filename).should == "hello, world!3456789hello, world!"
      end
    else
      IO.send(@method, @filename, "hello, world!", 0)
      File.read(@filename).should == "hello, world!34567890123456789"
      IO.send(@method, @filename, "hello, world!", 20)
      File.read(@filename).should == "hello, world!3456789hello, world!"
    end
  end

  it "doesn't truncate and writes at the given offset after passing empty opts" do
    if @method == :write
      NATFIXME "Add offset to IO.#{@method}", exception: ArgumentError, message: 'wrong number of arguments (given 3, expected 2)' do
        IO.send(@method, @filename, "hello world!", 1, **{})
        File.read(@filename).should == "0hello world!34567890123456789"
      end
    else
      IO.send(@method, @filename, "hello world!", 1, **{})
      File.read(@filename).should == "0hello world!34567890123456789"
    end
  end

  it "accepts a :mode option" do
    if @method == :write
      NATFIXME 'Keyword arguments (exact exception differs between callers)' do
        IO.send(@method, @filename, "hello, world!", mode: 'a')
        File.read(@filename).should == "012345678901234567890123456789hello, world!"
        IO.send(@method, @filename, "foo", 2, mode: 'w')
        File.read(@filename).should == "\0\0foo"
      end
    else
      IO.send(@method, @filename, "hello, world!", mode: 'a')
      File.read(@filename).should == "012345678901234567890123456789hello, world!"
      IO.send(@method, @filename, "foo", 2, mode: 'w')
      File.read(@filename).should == "\0\0foo"
    end
  end

  it "accepts a :flags option without :mode one" do
    if @method == :write
      NATFIXME 'Keyword arguments', exception: ArgumentError, message: 'wrong number of arguments (given 3, expected 2)' do
        IO.send(@method, @filename, "hello, world!", flags: File::CREAT)
        File.read(@filename).should == "hello, world!"
      end
    else
      IO.send(@method, @filename, "hello, world!", flags: File::CREAT)
      File.read(@filename).should == "hello, world!"
    end
  end

  it "raises an error if readonly mode is specified" do
    if @method == :write
      NATFIXME 'Keyword arguments', exception: SpecFailedException do
        -> { IO.send(@method, @filename, "abcde", mode: "r") }.should raise_error(IOError)
      end
    else
      -> { IO.send(@method, @filename, "abcde", mode: "r") }.should raise_error(IOError)
    end
  end

  it "truncates if empty :opts provided and offset skipped" do
    IO.send(@method, @filename, "hello, world!", **{})
    File.read(@filename).should == "hello, world!"
  end
end
