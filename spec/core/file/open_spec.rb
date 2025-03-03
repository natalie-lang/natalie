# encoding: utf-8

require_relative '../../spec_helper'
require_relative 'fixtures/common'
require_relative 'shared/open'

describe "File.open" do
  before :all do
    @file = tmp("file_open.txt")
    @unicode_path = tmp("こんにちは.txt")
    @nonexistent = tmp("fake.txt")
    rm_r @file, @nonexistent
  end

  before :each do
    ScratchPad.record []

    @fh = @fd = nil
    @flags = File::CREAT | File::TRUNC | File::WRONLY
    touch @file
  end

  after :each do
    @fh.close if @fh and not @fh.closed?
    rm_r @file, @unicode_path, @nonexistent
  end

  describe "with a block" do
    it "does not raise error when file is closed inside the block" do
      @fh = File.open(@file) { |fh| fh.close; fh }
      @fh.should.closed?
    end

    it "invokes close on an opened file when exiting the block" do
      File.open(@file, 'r') { |f| FileSpecs.make_closer f }

      ScratchPad.recorded.should == [:file_opened, :file_closed]
    end

    it "propagates non-StandardErrors produced by close" do
      -> {
        File.open(@file, 'r') { |f| FileSpecs.make_closer f, Exception }
      }.should raise_error(Exception)

      ScratchPad.recorded.should == [:file_opened, :file_closed]
    end

    it "propagates StandardErrors produced by close" do
      -> {
        File.open(@file, 'r') { |f| FileSpecs.make_closer f, StandardError }
      }.should raise_error(StandardError)

      ScratchPad.recorded.should == [:file_opened, :file_closed]
    end

    it "does not propagate IOError with 'closed stream' message produced by close" do
      File.open(@file, 'r') { |f| FileSpecs.make_closer f, IOError.new('closed stream') }

      ScratchPad.recorded.should == [:file_opened, :file_closed]
    end
  end

  it "opens the file (basic case)" do
    @fh = File.open(@file)
    @fh.should be_kind_of(File)
    File.should.exist?(@file)
  end

  it "opens the file with unicode characters" do
    @fh = File.open(@unicode_path, "w")
    @fh.should be_kind_of(File)
    File.should.exist?(@unicode_path)
  end

  it "opens a file when called with a block" do
    File.open(@file) { |fh| }
    File.should.exist?(@file)
  end

  it "opens with mode string" do
    @fh = File.open(@file, 'w')
    @fh.should be_kind_of(File)
    File.should.exist?(@file)
  end

  it "opens a file with mode string and block" do
    File.open(@file, 'w') { |fh| }
    File.should.exist?(@file)
  end

  it "opens a file with mode num" do
    @fh = File.open(@file, @flags)
    @fh.should be_kind_of(File)
    File.should.exist?(@file)
  end

  it "opens a file with mode num and block" do
    File.open(@file, 'w') { |fh| }
    File.should.exist?(@file)
  end

  it "opens a file with mode and permission as nil" do
    @fh = File.open(@file, nil, nil)
    @fh.should be_kind_of(File)
  end

  # For this test we delete the file first to reset the perms
  it "opens the file when passed mode, num and permissions" do
    rm_r @file
    File.umask(0011)
    @fh = File.open(@file, @flags, 0755)
    @fh.should be_kind_of(File)
    platform_is_not :windows do
      @fh.lstat.mode.to_s(8).should == "100744"
    end
    File.should.exist?(@file)
  end

  # For this test we delete the file first to reset the perms
  it "opens the file when passed mode, num, permissions and block" do
    rm_r @file
    File.umask(0022)
    File.open(@file, "w", 0755){ |fh| }
    platform_is_not :windows do
      File.stat(@file).mode.to_s(8).should == "100755"
    end
    File.should.exist?(@file)
  end

  it "creates the file and returns writable descriptor when called with 'w' mode and r-o permissions" do
    # it should be possible to write to such a file via returned descriptor,
    # even though the file permissions are r-r-r.

    File.open(@file, "w", 0444) { |f| f.write("test") }
    File.read(@file).should == "test"
  end

  platform_is_not :windows do
    it "opens the existing file, does not change permissions even when they are specified" do
      File.chmod(0664, @file)
      orig_perms = File.stat(@file).mode.to_s(8)
      File.open(@file, "w", 0444) { |f| f.write("test") }

      File.stat(@file).mode.to_s(8).should == orig_perms
      File.read(@file).should == "test"
    end
  end

  platform_is_not :windows do
    as_user do
      it "creates a new write-only file when invoked with 'w' and '0222'" do
        rm_r @file
        File.open(@file, 'w', 0222) {}
        File.readable?(@file).should == false
        File.writable?(@file).should == true
      end
    end
  end

  it "opens the file when call with fd" do
    @fh = File.open(@file)
    fh_copy = File.open(@fh.fileno)
    fh_copy.autoclose = false
    fh_copy.should be_kind_of(File)
    File.should.exist?(@file)
  end

  it "opens a file that no exists when use File::WRONLY mode" do
    -> { File.open(@nonexistent, File::WRONLY) }.should raise_error(Errno::ENOENT)
  end

  it "opens a file that no exists when use File::RDONLY mode" do
    -> { File.open(@nonexistent, File::RDONLY) }.should raise_error(Errno::ENOENT)
  end

  it "opens a file that no exists when use 'r' mode" do
    -> { File.open(@nonexistent, 'r') }.should raise_error(Errno::ENOENT)
  end

  it "opens a file that no exists when use File::EXCL mode" do
    -> { File.open(@nonexistent, File::EXCL) }.should raise_error(Errno::ENOENT)
  end

  it "opens a file that no exists when use File::NONBLOCK mode" do
    -> { File.open(@nonexistent, File::NONBLOCK) }.should raise_error(Errno::ENOENT)
  end

  platform_is_not :openbsd, :windows do
    it "opens a file that no exists when use File::TRUNC mode" do
      -> { File.open(@nonexistent, File::TRUNC) }.should raise_error(Errno::ENOENT)
    end
  end

  platform_is :openbsd, :windows do
    it "does not open a file that does no exists when using File::TRUNC mode" do
      -> { File.open(@nonexistent, File::TRUNC) }.should raise_error(Errno::EINVAL)
    end
  end

  platform_is_not :windows do
    it "opens a file that no exists when use File::NOCTTY mode" do
      -> { File.open(@nonexistent, File::NOCTTY) }.should raise_error(Errno::ENOENT)
    end
  end

  it "opens a file that no exists when use File::CREAT mode" do
    @fh = File.open(@nonexistent, File::CREAT) { |f| f }
    @fh.should be_kind_of(File)
    File.should.exist?(@file)
  end

  it "opens a file that no exists when use 'a' mode" do
    @fh = File.open(@nonexistent, 'a') { |f| f }
    @fh.should be_kind_of(File)
    File.should.exist?(@file)
  end

  it "opens a file that no exists when use 'w' mode" do
    @fh = File.open(@nonexistent, 'w') { |f| f }
    @fh.should be_kind_of(File)
    File.should.exist?(@file)
  end

  # Check the grants associated to the different open modes combinations.
  it "raises an ArgumentError exception when call with an unknown mode" do
    -> { File.open(@file, "q") }.should raise_error(ArgumentError)
  end

  it "can read in a block when call open with RDONLY mode" do
    File.open(@file, File::RDONLY) do |f|
      f.gets.should == nil
    end
  end

  it "can read in a block when call open with 'r' mode" do
    File.open(@file, "r") do |f|
      f.gets.should == nil
    end
  end

  it "raises an IO exception when write in a block opened with RDONLY mode" do
    File.open(@file, File::RDONLY) do |f|
      -> { f.puts "writing ..." }.should raise_error(IOError)
    end
  end

  it "raises an IO exception when write in a block opened with 'r' mode" do
    File.open(@file, "r") do |f|
      -> { f.puts "writing ..." }.should raise_error(IOError)
    end
  end

  it "can't write in a block when call open with File::WRONLY||File::RDONLY mode" do
    File.open(@file, File::WRONLY|File::RDONLY ) do |f|
      f.puts("writing").should == nil
    end
  end

  it "can't read in a block when call open with File::WRONLY||File::RDONLY mode" do
    -> {
      File.open(@file, File::WRONLY|File::RDONLY ) do |f|
        f.gets.should == nil
      end
    }.should raise_error(IOError)
  end

  it "can write in a block when call open with WRONLY mode" do
    File.open(@file, File::WRONLY) do |f|
      f.puts("writing").should == nil
    end
  end

  it "can write in a block when call open with 'w' mode" do
    File.open(@file, "w") do |f|
      f.puts("writing").should == nil
    end
  end

  it "raises an IOError when read in a block opened with WRONLY mode" do
    File.open(@file, File::WRONLY) do |f|
      -> { f.gets  }.should raise_error(IOError)
    end
  end

  it "raises an IOError when read in a block opened with 'w' mode" do
    File.open(@file, "w") do |f|
      -> { f.gets   }.should raise_error(IOError)
    end
  end

  it "raises an IOError when read in a block opened with 'a' mode" do
    File.open(@file, "a") do |f|
      -> { f.gets  }.should raise_error(IOError)
    end
  end

  it "raises an IOError when read in a block opened with 'a' mode" do
    File.open(@file, "a") do |f|
      f.puts("writing").should == nil
      -> { f.gets }.should raise_error(IOError)
    end
  end

  it "raises an IOError when read in a block opened with 'a' mode" do
    File.open(@file, File::WRONLY|File::APPEND ) do |f|
      -> { f.gets }.should raise_error(IOError)
    end
  end

  it "raises an IOError when read in a block opened with File::WRONLY|File::APPEND mode" do
    File.open(@file, File::WRONLY|File::APPEND ) do |f|
      f.puts("writing").should == nil
      -> { f.gets }.should raise_error(IOError)
    end
  end

  it "raises an IOError when write in a block opened with File::RDONLY|File::APPEND mode" do
    -> {
      File.open(@file, File::RDONLY|File::APPEND ) do |f|
        f.puts("writing")
      end
    }.should raise_error(IOError)
  end

  it "can read and write in a block when call open with RDWR mode" do
    File.open(@file, File::RDWR) do |f|
      f.gets.should == nil
      f.puts("writing").should == nil
      f.rewind
      f.gets.should == "writing\n"
    end
  end

  it "can't read in a block when call open with File::EXCL mode" do
    -> {
      File.open(@file, File::EXCL) do |f|
        f.puts("writing").should == nil
      end
    }.should raise_error(IOError)
  end

  it "can read in a block when call open with File::EXCL mode" do
    File.open(@file, File::EXCL) do |f|
      f.gets.should == nil
    end
  end

  it "can read and write in a block when call open with File::RDWR|File::EXCL mode" do
    File.open(@file, File::RDWR|File::EXCL) do |f|
      f.gets.should == nil
      f.puts("writing").should == nil
      f.rewind
      f.gets.should == "writing\n"
    end
  end

  it "raises an Errorno::EEXIST if the file exists when open with File::CREAT|File::EXCL" do
    -> {
      File.open(@file, File::CREAT|File::EXCL) do |f|
        f.puts("writing")
      end
    }.should raise_error(Errno::EEXIST)
  end

  it "creates a new file when use File::WRONLY|File::APPEND mode" do
    @fh = File.open(@file, File::WRONLY|File::APPEND)
    @fh.should be_kind_of(File)
    File.should.exist?(@file)
  end

  it "opens a file when use File::WRONLY|File::APPEND mode" do
    File.open(@file, File::WRONLY) do |f|
      f.puts("hello file")
    end
    File.open(@file, File::RDWR|File::APPEND) do |f|
      f.puts("bye file")
      f.rewind
      f.gets.should == "hello file\n"
      f.gets.should == "bye file\n"
      f.gets.should == nil
    end
  end

  it "raises an IOError if the file exists when open with File::RDONLY|File::APPEND" do
    -> {
      File.open(@file, File::RDONLY|File::APPEND) do |f|
        f.puts("writing").should == nil
      end
    }.should raise_error(IOError)
  end

  platform_is_not :openbsd, :windows do
    it "truncates the file when passed File::TRUNC mode" do
      File.open(@file, File::RDWR) { |f| f.puts "hello file" }
      @fh = File.open(@file, File::TRUNC)
      @fh.gets.should == nil
    end

    it "can't read in a block when call open with File::TRUNC mode" do
      File.open(@file, File::TRUNC) do |f|
        f.gets.should == nil
      end
    end
  end

  it "opens a file when use File::WRONLY|File::TRUNC mode" do
    fh1 = File.open(@file, "w")
    begin
      @fh = File.open(@file, File::WRONLY|File::TRUNC)
      @fh.should be_kind_of(File)
      File.should.exist?(@file)
    ensure
      fh1.close
    end
  end

  platform_is_not :openbsd, :windows do
    it "can't write in a block when call open with File::TRUNC mode" do
      -> {
        File.open(@file, File::TRUNC) do |f|
          f.puts("writing")
        end
      }.should raise_error(IOError)
    end

    it "raises an Errorno::EEXIST if the file exists when open with File::RDONLY|File::TRUNC" do
      -> {
        File.open(@file, File::RDONLY|File::TRUNC) do |f|
          f.puts("writing").should == nil
        end
      }.should raise_error(IOError)
    end
  end

  platform_is :openbsd, :windows do
    it "can't write in a block when call open with File::TRUNC mode" do
      -> {
        File.open(@file, File::TRUNC) do |f|
          f.puts("writing")
        end
      }.should raise_error(Errno::EINVAL)
    end

    it "raises an Errorno::EEXIST if the file exists when open with File::RDONLY|File::TRUNC" do
      -> {
        File.open(@file, File::RDONLY|File::TRUNC) do |f|
          f.puts("writing").should == nil
        end
      }.should raise_error(Errno::EINVAL)
    end
  end

  platform_is_not :windows do
    as_user do
      it "raises an Errno::EACCES when opening non-permitted file" do
        @fh = File.open(@file, "w")
        @fh.chmod(000)
        -> { fh1 = File.open(@file); fh1.close }.should raise_error(Errno::EACCES)
      end
    end
  end

  as_user do
    it "raises an Errno::EACCES when opening read-only file" do
      @fh = File.open(@file, "w")
      @fh.chmod(0444)
      -> { File.open(@file, "w") }.should raise_error(Errno::EACCES)
    end
  end

  it "opens a file for binary read" do
    @fh = File.open(@file, "rb")
    @fh.should be_kind_of(File)
    File.should.exist?(@file)
  end

  it "opens a file for binary write" do
    @fh = File.open(@file, "wb")
    @fh.should be_kind_of(File)
    File.should.exist?(@file)
  end

  it "opens a file for read-write and truncate the file" do
    File.open(@file, "w") { |f| f.puts "testing" }
    File.size(@file).should > 0
    File.open(@file, "w+") do |f|
      f.pos.should == 0
      f.should.eof?
    end
    File.size(@file).should == 0
  end

  it "opens a file for binary read-write starting at the beginning of the file" do
    File.open(@file, "w") { |f| f.puts "testing" }
    File.size(@file).should > 0
    File.open(@file, "rb+") do |f|
      f.binmode?.should == true
      f.external_encoding.should == Encoding::ASCII_8BIT
      f.pos.should == 0
      f.should_not.eof?
    end
    File.open(@file, "r+b") do |f|
      f.binmode?.should == true
      f.external_encoding.should == Encoding::ASCII_8BIT
      f.pos.should == 0
      f.should_not.eof?
    end
  end

  it "opens a file for binary read-write and truncate the file" do
    File.open(@file, "w") { |f| f.puts "testing" }
    File.size(@file).should > 0
    File.open(@file, "wb+") do |f|
      f.pos.should == 0
      f.should.eof?
    end
    File.size(@file).should == 0
  end

  platform_is :linux do
    guard -> { defined?(File::TMPFILE) } do
      it "creates an unnamed temporary file with File::TMPFILE" do
        dir = tmp("tmpfilespec")
        mkdir_p dir
        begin
          Dir["#{dir}/*"].should == []
          File.open(dir, "r+", flags: File::TMPFILE) do |io|
            io.write("ruby")
            io.flush
            io.rewind
            io.read.should == "ruby"
            Dir["#{dir}/*"].should == []
          end
        rescue Errno::EOPNOTSUPP
          skip "no support from the filesystem"
        rescue Errno::EINVAL, Errno::EISDIR
          skip "presumably bug in glibc"
        ensure
          rm_r dir
        end
      end
    end
  end

  it "raises a TypeError if passed a filename that is not a String or Integer type" do
    -> { File.open(true)  }.should raise_error(TypeError)
    -> { File.open(false) }.should raise_error(TypeError)
    -> { File.open(nil)   }.should raise_error(TypeError)
  end

  it "raises a SystemCallError if passed an invalid Integer type" do
    -> { File.open(-1)    }.should raise_error(SystemCallError)
  end

  it "raises an ArgumentError if passed the wrong number of arguments" do
    -> { File.open(@file, File::CREAT, 0755, 'test') }.should raise_error(ArgumentError)
  end

  it "raises an ArgumentError if passed an invalid string for mode" do
    -> { File.open(@file, 'fake') }.should raise_error(ArgumentError)
  end

  it "defaults external_encoding to BINARY for binary modes" do
    File.open(@file, 'rb') {|f| f.external_encoding.should == Encoding::BINARY}
    File.open(@file, 'wb+') {|f| f.external_encoding.should == Encoding::BINARY}
  end

  it "accepts options as a keyword argument" do
    @fh = File.open(@file, 'w', 0755, flags: File::CREAT)
    @fh.should be_an_instance_of(File)

    -> {
      File.open(@file, 'w', 0755, {flags: File::CREAT})
    }.should raise_error(ArgumentError, "wrong number of arguments (given 4, expected 1..3)")
  end

  it "uses the second argument as an options Hash" do
    @fh = File.open(@file, mode: "r")
    @fh.should be_an_instance_of(File)
  end

  it "calls #to_hash to convert the second argument to a Hash" do
    options = mock("file open options")
    options.should_receive(:to_hash).and_return({ mode: "r" })

    @fh = File.open(@file, **options)
  end

  it "accepts extra flags as a keyword argument and combine with a string mode" do
    -> {
      File.open(@file, "w", flags: File::EXCL) { }
    }.should raise_error(Errno::EEXIST)

    -> {
      File.open(@file, mode: "w", flags: File::EXCL) { }
    }.should raise_error(Errno::EEXIST)
  end

  it "accepts extra flags as a keyword argument and combine with an integer mode" do
    -> {
      File.open(@file, File::WRONLY | File::CREAT, flags: File::EXCL) { }
    }.should raise_error(Errno::EEXIST)
  end

  platform_is_not :windows do
    describe "on a FIFO" do
      before :each do
        @fifo = tmp("File_open_fifo")
        File.mkfifo(@fifo)
      end

      after :each do
        rm_r @fifo
      end

      it "opens it as a normal file" do
        file_w, file_r, read_bytes, written_length = nil

        # open in threads, due to blocking open and writes
        writer = Thread.new do
          file_w = File.open(@fifo, 'w')
          written_length = file_w.syswrite('hello')
        end
        reader = Thread.new do
          file_r = File.open(@fifo, 'r')
          read_bytes = file_r.sysread(5)
        end

        begin
          writer.join
          reader.join

          written_length.should == 5
          read_bytes.should == 'hello'
        ensure
          file_w.close if file_w
          file_r.close if file_r
        end
      end
    end
  end

  it "raises ArgumentError if mixing :newline and binary mode" do
    NATFIXME 'newline argument', exception: SpecFailedException do
      -> {
        File.open(@file, "rb", newline: :universal) {}
      }.should raise_error(ArgumentError, "newline decorator with binary mode")
    end
  end

  context "'x' flag" do
    before :each do
      @xfile = tmp("x-flag")
      rm_r @xfile
    end

    after :each do
      rm_r @xfile
    end

    it "does nothing if the file doesn't exist" do
      NATFIXME "'x' flag", exception: ArgumentError, message: 'invalid access mode wx' do
        File.open(@xfile, "wx") { |f| f.write("content") }
        File.read(@xfile).should == "content"
      end
    end

    it "throws a Errno::EEXIST error if the file exists" do
      touch @xfile
      NATFIXME "'x' flag", exception: SpecFailedException do
        -> { File.open(@xfile, "wx") }.should raise_error(Errno::EEXIST)
      end
    end

    it "can't be used with 'r' and 'a' flags" do
      -> { File.open(@xfile, "rx") }.should raise_error(ArgumentError, 'invalid access mode rx')
      -> { File.open(@xfile, "ax") }.should raise_error(ArgumentError, 'invalid access mode ax')
    end
  end
end

describe "File.open when passed a file descriptor" do
  before do
    @content = "File#open when passed a file descriptor"
    @name = tmp("file_open_with_fd.txt")
    @fd = new_fd @name, "w:utf-8"
    @file = nil
  end

  after do
    @file.close if @file and not @file.closed?
    rm_r @name
  end

  it "opens a file" do
    @file = File.open(@fd, "w")
    @file.should be_an_instance_of(File)
    @file.fileno.should equal(@fd)
    @file.write @content
    NATFIXME 'Implement File#flush', exception: NoMethodError, message: /undefined method [`']flush'/ do
      @file.flush
    end
    @file.close # NATFIXME: This should flush the file as well
    File.read(@name).should == @content
  end

  it "opens a file when passed a block" do
    @file = File.open(@fd, "w") do |f|
      f.should be_an_instance_of(File)
      f.fileno.should equal(@fd)
      f.write @content
      f
    end
    File.read(@name).should == @content
  end
end

platform_is_not :windows do
  describe "File.open" do
    it_behaves_like :open_directory, :open
  end
end
