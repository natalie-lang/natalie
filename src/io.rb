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
          obj.fsync unless obj.tty? || obj.respond_to?(:setsockopt)
          obj.close
        rescue IOError => e
          raise unless e.message == 'closed stream'
        end
      end
    end

    def foreach(path, *args, **opts)
      if path.to_s.start_with?('|')
        raise NotImplementedError, 'no support for pipe in IO.foreach'
      end
      return enum_for(:foreach, path, *args, **opts) unless block_given?

      mode = opts.delete(:mode) || 'r'
      chomp = opts.delete(:chomp)
      File.open(path, mode, **opts) do |io|
        io.each_line(*args, **opts.merge(chomp: chomp)) do |line|
          $_ = line
          yield line
        end
      end
      $_ = nil
    end

    def readlines(path, *args, **opts, &block)
      if path.to_s.start_with?('|')
        raise NotImplementedError, 'no support for pipe in IO.readlines'
      end
      mode = opts.delete(:mode) || 'r'
      chomp = opts.delete(:chomp)
      File.open(path, mode, **opts) do |io|
        io.each_line(*args, **opts.merge(chomp: chomp), &block).to_a
      end
    end
  end

  def printf(format_string, *arguments)
    print(Kernel.sprintf(format_string, *arguments))
  end

  def readlines(...)
    each_line(...).to_a
  end

  def each(*args, **opts)
    return enum_for(:each_line, *args, **opts) unless block_given?

    if args.size == 2
      sep, limit = args
    elsif args.size == 1
      arg = args.first
      if arg.nil?
        sep = nil
      elsif arg.respond_to?(:to_int)
        limit = arg
        sep = $/
      elsif arg.respond_to?(:to_str)
        sep = arg
      else
        raise TypeError, "no implicit conversion of #{args[0].class} into Integer"
      end
    elsif args.empty?
      sep = $/
      limit = nil
    else
      raise ArgumentError, "wrong number of arguments (given #{args.size}, expected 0..2)"
    end

    sep = sep.to_str if sep.respond_to?(:to_str)
    raise TypeError, "no implicit conversion of #{sep.class} into String" if sep && !sep.is_a?(String)

    limit = limit.to_int if limit.respond_to?(:to_int)
    raise TypeError, "no implicit conversion of #{limit.class} into Integer" if limit && !limit.is_a?(Integer)

    limit = nil if limit&.negative?
    raise ArgumentError, "invalid limit: 0 for #{__method__}" if limit&.zero?

    chomp = opts.delete(:chomp)

    if sep.nil? && limit.nil?
      yield read
      return
    end

    if sep == ''
      sep = "\n\n"
      skip_leading_newlines = true
    end

    buf = String.new('', encoding: Encoding.default_external)
    get_bytes = lambda do |size|
      buf << read(1024) while !eof? && buf.bytesize < size
      return nil if eof? && buf.empty?
      buf.byteslice(0, size).force_encoding(Encoding.default_external)
    end
    advance = lambda do |size|
      buf = buf.byteslice(size..)
      if skip_leading_newlines
        num_leading_newlines = buf.match(/^\n+/).to_s.size
        buf = buf.byteslice(num_leading_newlines..)
      end
    end

    $. = 0
    loop do
      line = get_bytes.(limit || 1024)
      break unless line
      tries = 0
      until line.valid_encoding? || tries >= 4
        tries += 1
        line = get_bytes.(limit + tries)
      end
      if sep && (sep_index = line.index(sep))
        line = line[...(sep_index + sep.size)]
      end
      advance.(line.bytesize)
      line.chomp! if sep && sep_index && chomp

      $. += 1
      self.lineno += 1
      yield line
    end

    self
  end

  alias each_line each

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
