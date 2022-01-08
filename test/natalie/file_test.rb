require_relative '../spec_helper'

describe 'File' do
  it 'is an IO object' do
    f = File.new('test/support/file.txt')
    f.should be_kind_of(IO)
    f.close
  end

  it 'raises ENOENT when opening a file that does not exist' do
    -> { File.new('file_does_not_exist.txt') }.should raise_error(Errno::ENOENT)
  end

  describe 'SEPARATOR' do
    it 'is hardcoded to / for now' do
      File::SEPARATOR.should == '/'
    end
  end

  describe '.new' do
    it 'returns the File object' do
      path = File.expand_path('../tmp/file.txt', __dir__)
      file = File.new(path, 'w')
      bytes_written = file.write('hello world')
      file.close
      bytes_written.should == 11
      actual = File.read(path)
      actual.should == 'hello world'
    end

    it 'raises an error when trying to read/write a bad path' do
      -> { File.new('/tmp/should_not_exist/file.txt') }.should raise_error(Errno::ENOENT)
      -> { File.new('/tmp/should_not_exist.txt', 'r') }.should raise_error(Errno::ENOENT)
    end
  end

  describe '.open' do
    it 'without a block it returns the File object' do
      path = File.expand_path('../tmp/file.txt', __dir__)
      file = File.open(path, 'w')
      bytes_written = file.write('hello world')
      file.close
      bytes_written.should == 11
      actual = File.read(path)
      actual.should == 'hello world'
    end

    it 'accepts a block' do
      path = File.expand_path('../tmp/file.txt', __dir__)
      bytes_written = File.open(path, 'w') { |f| f.write('hello world') }
      actual = File.read(path)
      actual.should == 'hello world'
      bytes_written.should == 11
    end

    it 'raises an error when trying to read/write a bad path' do
      -> { File.open('/tmp/should_not_exist/file.txt') }.should raise_error(Errno::ENOENT)
      -> { File.open('/tmp/should_not_exist.txt', 'r') }.should raise_error(Errno::ENOENT)
    end

    it 'closes the file when the block has an exception' do
      path = File.expand_path('../tmp/file.txt', __dir__)
      file = nil
      begin
        File.open(path, 'w') do |f|
          file = f
          f.puts('hello world')
          raise 'some error'
        end
      rescue StandardError
      end
      file.closed?.should == true
    end
  end

  describe '#read' do
    it 'reads the entire file' do
      f = File.new('test/support/file.txt')
      f.read.should == "foo bar baz\n"
      f.read.should == ''
      f.close
    end

    it 'reads specified number of bytes' do
      f = File.new('test/support/file.txt')
      f.read(4).should == 'foo '
      f.read(4).should == 'bar '
      f.read(10).should == "baz\n"
      f.read(4).should be_nil
      f.close
    end
  end

  describe '#write' do
    it 'writes to the file using an integer mode' do
      f = File.new('test/tmp/file_write_test.txt', File::CREAT | File::WRONLY | File::TRUNC)
      f.write('write ')
      f.close
      f = File.new('test/tmp/file_write_test.txt', File::CREAT | File::WRONLY | File::APPEND)
      f.write('append')
      f.close
      f = File.new('test/tmp/file_write_test.txt')
      f.read.should == 'write append'
      f.close
    end

    it 'writes to the file using a string mode' do
      f = File.new('test/tmp/file_write_test.txt', 'w')
      f.write('write ')
      f.close
      f = File.new('test/tmp/file_write_test.txt', 'a')
      f.write('append')
      f.close
      f = File.new('test/tmp/file_write_test.txt')
      f.read.should == 'write append'
      f.close
    end
  end

  describe '#seek' do
    it 'seeks to an absolute position' do
      f = File.new('test/support/file.txt')
      f.seek(4)
      f.seek(4).should == 0
      f.read(3).should == 'bar'
      f.seek(8, :SET)
      f.read(3).should == 'baz'
      f.seek(4, IO::SEEK_SET)
      f.read(3).should == 'bar'
      f.close
    end

    it 'seeks to an offset position from current' do
      f = File.new('test/support/file.txt')
      f.seek(4)
      f.seek(4, :CUR).should == 0
      f.read(3).should == 'baz'
      f.seek(4)
      f.seek(-4, IO::SEEK_CUR)
      f.read(3).should == 'foo'
      f.close
    end

    it 'seeks to an offset position from end' do
      f = File.new('test/support/file.txt')
      f.seek(-4, :END).should == 0
      f.read(3).should == 'baz'
      f.seek(-8, IO::SEEK_END)
      f.read(3).should == 'bar'
      f.close
    end
  end

  describe '#rewind' do
    it 'seeks to the beginning' do
      f = File.new('test/support/file.txt')
      f.read.should == "foo bar baz\n"
      f.rewind
      f.read.should == "foo bar baz\n"
      f.close
    end
  end

  describe '#fileno' do
    it 'returns the file descriptor number' do
      f = File.new('test/support/file.txt')
      f.fileno.should be_kind_of(Integer)
      f.close
    end
  end

  describe '#close' do
    it 'closes the file' do
      f = File.open('test/support/file.txt')
      f.close
      -> { f.close }.should_not raise_error(nil)
      -> { f.read }.should raise_error(IOError, 'closed stream')
    end
  end

  describe '.expand_path' do
    it 'returns the absolute path given a relative one' do
      File.expand_path('test/spec_helper.rb').should =~ %r{^/.*natalie/test/spec_helper\.rb$}
      File.expand_path('/spec_helper.rb').should == '/spec_helper.rb'
      File.expand_path('../spec_helper.rb', __dir__).should =~ %r{^/.*natalie/test/spec_helper\.rb$}
      File.expand_path('..', __dir__).should =~ %r{^/.*natalie/test$}
      File.expand_path('.', __dir__).should =~ %r{^/.*natalie/test/natalie$}
      File.expand_path('/foo/./bar').should == '/foo/bar'
      File.expand_path('/foo/bar/.').should == '/foo/bar'
      File.expand_path('/foo/bar', '/baz').should == '/foo/bar'
    end
  end

  describe '.unlink' do
    it 'deletes the given file path' do
      path = File.expand_path('../tmp/file_to_delete.txt', __dir__)
      File.open(path, 'w') { |f| f.write('hello world') }
      File.unlink(path)
      -> { File.open(path, 'r') }.should raise_error(Errno::ENOENT)
      -> { File.unlink(path) }.should raise_error(Errno::ENOENT)
    end
  end

  describe '.exist?' do
    it 'returns true if the path exists' do
      File.exist?(__dir__).should be_true
      File.exist?(__FILE__).should be_true
      File.exist?('should_not_exist').should be_false
    end
  end

  describe '.dirname' do
    it 'returns the directory of a given path sans filename' do
      File.dirname('foo/bar.txt').should == 'foo'
      File.dirname('/foo/bar/baz.txt').should == '/foo/bar'
      File.dirname('/foo/bar/baz.txt/').should == '/foo/bar'
      File.dirname('/foo/bar/baz').should == '/foo/bar'
      File.dirname('/foo/bar/').should == '/foo'
      File.dirname('/foo/bar/ ').should == '/foo/bar'
      File.dirname('/foo/bar').should == '/foo'
      File.dirname('  /foo/bar').should == '  /foo'
      File.dirname('../foo/bar').should == '../foo'
      File.dirname('../foo/bar/baz.md').should == '../foo/bar'
      File.dirname('   ../foo/bar/baz.md  ').should == '   ../foo/bar'
      File.dirname('').should == '.'
      File.dirname(' ').should == '.'
      File.dirname('/').should == '/'
      File.dirname('/  ').should == '/'
      File.dirname('  /').should == '.'
      File.dirname('  /  ').should == '  '
      File.dirname('./').should == '.'
      File.dirname('./  ').should == '.'
      File.dirname('../').should == '.'
      File.dirname('../  ').should == '..'
      File.dirname('../../').should == '..'
    end
  end
end
