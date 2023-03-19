class IO
  SEEK_SET = 0
  SEEK_CUR = 1
  SEEK_END = 2

  def rewind
    seek(0)
  end

  def each
    while (line = gets)
      yield line
    end
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
