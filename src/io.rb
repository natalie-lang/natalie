class IO
  class << self
    alias for_fd new

    def open(*args, **kwargs)
      obj = new(*args, **kwargs)
      return obj unless block_given?

      begin
        yield(obj)
      ensure
        begin
          obj.fsync unless obj.tty?
          obj.close
        rescue IOError => e
          raise unless e.message == 'closed stream'
        end
      end
    end
  end

  SEEK_SET = 0
  SEEK_CUR = 1
  SEEK_END = 2
  SEEK_DATA = 3
  SEEK_HOLE = 4

  def each
    while (line = gets)
      yield line
    end
  end

  def printf(format_string, *arguments)
    print(Kernel.sprintf(format_string, *arguments))
  end

  # The following are used in IO.select

  module WaitReadable; end
  module WaitWritable; end

  class EAGAINWaitReadable < Errno::EAGAIN
    include IO::WaitReadable
  end

  class EAGAINWaitWritable < Errno::EAGAIN
    include IO::WaitWritable
  end

  class EWOULDBLOCKWaitReadable < Errno::EWOULDBLOCK
    include IO::WaitReadable
  end

  class EWOULDBLOCKWaitWritable < Errno::EWOULDBLOCK
    include IO::WaitWritable
  end

  class EINPROGRESSWaitReadable < Errno::EINPROGRESS
    include IO::WaitReadable
  end

  class EINPROGRESSWaitWritable < Errno::EINPROGRESS
    include IO::WaitWritable
  end
end
