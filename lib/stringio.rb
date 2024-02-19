class StringIO
  VERSION = '3.1.0'.freeze

  attr_reader :string
  attr_accessor :lineno

  private def initialize(string = '', mode = nil)
    unless string.is_a? String
      string = string.to_str
    end
    @string = string
    @index = 0
    @lineno = 0

    unless mode
      if string.frozen?
        mode = 'r'
      else
        mode = 'r+'
      end
    end

    unless mode.is_a? String
      mode = mode.to_str
    end
    @mode = mode

    __set_closed

    if !closed_write? && string.frozen?
      raise Errno::EACCES, 'Permission denied'
    end

    warn('warning: StringIO::new() does not take block; use StringIO::open() instead') if block_given?
  end

  def binmode
    @string.force_encoding(Encoding::ASCII_8BIT)
    self
  end

  def close
    @read_closed = true
    @write_closed = true
    nil
  end

  def closed?
    closed_read? && closed_write?
  end

  def close_read
    raise IOError, 'closing non-duplex IO for reading' unless @mode.include?('r')
    @read_closed = true
    nil
  end

  def closed_read?
    !!@read_closed
  end

  def close_write
    raise IOError, 'closing non-duplex IO for writing' unless @mode == 'r+' || @mode.include?('w')
    @write_closed = true
    nil
  end

  def closed_write?
    !!@write_closed
  end

  def each(separator = $/, limit = nil, chomp: false)
    return enum_for(:each) unless block_given?
    __assert_not_read_closed

    until eof?
      yield __next_line(separator, limit, chomp: chomp)
    end

    self
  end
  alias each_line each

  def each_byte
    return enum_for(:each_byte) unless block_given?

    until eof?
      getc.each_byte { |b| yield b }
    end

    self
  end

  def each_char
    return enum_for(:each_char) unless block_given?

    until eof?
      yield getc
    end

    self
  end

  def each_codepoint
    return enum_for(:each_codepoint) unless block_given?

    each_char { |c| yield c.ord }
  end

  def eof?
    @index >= @string.length
  end
  alias eof eof?

  def external_encoding
    @external_encoding || @string.encoding
  end

  def fcntl
    raise NotImplementedError, 'fcntl() function is unimplemented on this machine'
  end

  def fileno
    nil
  end

  def flush
    self
  end

  def fsync
    0
  end

  def getbyte
    __assert_not_read_closed
    return nil if eof?
    byte = @string.getbyte(@index)
    @index += 1
    byte
  end

  def getc
    __assert_not_read_closed

    @string[@index].tap do
      @index += 1 unless eof?
    end
  end

  def gets(separator = $/, limit = nil, chomp: false)
    __assert_not_read_closed
    return $_ = nil if eof?

    $_ = __next_line(separator, limit, chomp: chomp)
  end

  def internal_encoding
    nil
  end

  def isatty
    false
  end
  alias tty? isatty

  def pid
    nil
  end

  def pos
    @index
  end
  alias tell pos

  def pos=(new_index)
    if new_index < 0
      raise Errno::EINVAL, 'Invalid argument'
    end

    @index = new_index
  end

  def printf(format_str, *args)
    write(Kernel.sprintf(format_str, *args))
    nil
  end

  def read(length = nil, out_string = nil)
    __assert_not_read_closed

    encoding = nil
    if length
      if !length.is_a?(Integer) && length.respond_to?(:to_int)
        length = length.to_int
      end

      unless length.is_a? Integer
        raise TypeError, "no implicit conversion of #{length.class} into Integer"
      end

      if length < 0
        raise ArgumentError, "negative length #{length} given"
      end

      return '' if length == 0
      return nil if eof?

      encoding = Encoding::BINARY
    else
      return '' if eof?

      length = @string.length - @index
    end

    if out_string
      if !out_string.is_a?(String) && out_string.respond_to?(:to_str)
        out_string = out_string.to_str
      end

      unless out_string.is_a? String
        raise TypeError, "no implicit conversion of #{out_string.class} into String"
      end
    end

    if @index + length > @string.length
      length = @string.length - @index
    end

    result = @string[@index..(@index + [length - 1, 0].max)]
    @index += length

    if out_string
      out_string.replace(result)
    else
      if encoding
        result = result.encode(encoding)
      end
      result
    end
  end

  def read_nonblock(length = nil, buffer = nil, exception: true)
    result = read(length, buffer)
    if length&.to_int&.positive? && (result.nil? || result.empty?)
      raise EOFError, 'end of file reached' if exception
      return nil
    end
    result
  end
  alias sysread read_nonblock

  def rewind
    @lineno = 0
    @index = 0
  end

  def set_encoding(external_encoding, _ = nil, **_options)
    external_encoding = Encoding.find(external_encoding) if external_encoding.is_a?(String)
    @external_encoding = external_encoding || Encoding.default_external
    unless @string.frozen?
      @string.force_encoding(@external_encoding)
    end
    self
  end

  def size
    @string.size
  end
  alias length size

  def string=(string)
    @string = string.is_a?(String) ? string : string.to_str
    rewind
  end

  def sync
    true
  end

  def sync=(arg)
    arg
  end

  def ungetbyte(integer)
    __assert_not_read_closed
    __assert_not_write_closed
    return if integer.nil?
    if integer.is_a?(String)
      if @index.zero?
        @string.prepend(integer)
      else
        integer.bytes.reverse.each do |byte|
          if @index.zero?
            @string.prepend(byte.chr)
          else
            @index -= 1
            @string.setbyte(@index, byte)
          end
        end
      end
    else
      integer &= 0xff
      if @index.zero?
        @string.prepend(integer.chr)
      else
        @index -= 1
        @string.setbyte(@index, integer)
      end
    end
    nil
  end

  def putc(argument)
    __assert_not_write_closed

    if !argument.is_a?(String)
      if !argument.respond_to?(:to_int)
        raise TypeError, "no implicit conversion of #{argument.class} into Integer"
      end

      argument = (argument.to_int % 256).chr
    end

    write(argument.to_s[0])

    argument
  end

  def write(argument)
    __assert_not_write_closed

    unless argument.is_a? String
      argument = argument.to_s
    end

    if __appending?
      @string << argument
      @index = @string.length
    elsif @index >= @string.length
      @string << "\000" * (@index - @string.length) << argument
      @index = @string.length
    else
      @string[@index, argument.length] = argument
      @index += argument.length
    end
    argument.bytes.size
  end

  alias syswrite write

  def write_nonblock(argument, exception: true)
    write(argument)
  end

  def <<(argument)
    write(argument)
    self
  end

  private def __assert_not_read_closed
    raise IOError, 'not opened for reading' if closed_read?
  end

  private def __assert_not_write_closed
    raise IOError, 'not opened for writing' if closed_write?
  end

  private def __appending?
    @mode[0] == 'a'
  end

  private def __set_closed
    if @mode[-1] == '+'
      @read_closed = false
      @write_closed = false
      return
    end

    case @mode[0]
    when 'r'
      @read_closed = false
      @write_closed = true
    when 'w', 'a'
      @read_closed = true
      @write_closed = false
    end
  end

  private def __next_line(separator, limit, chomp: false)
    return if eof?

    if limit && !limit.is_a?(Integer)
      limit = limit.to_int
    end

    initial_separator = separator
    if separator && !separator.is_a?(String)
      if separator.respond_to?(:to_str)
        separator = separator.to_str
      elsif separator.respond_to?(:to_int)
        separator = separator.to_int
      end
    end

    if separator.is_a? Integer
      limit = separator
      separator = $/
    end

    if limit == 0
      @lineno += 1
      return ''
    end

    if separator == ''
      separator = "\n\n"
    end

    if separator
      line_ending = @string[@index..].index(separator)
    end

    if line_ending
      if chomp
        line_ending += @index - 1
      else
        line_ending += @index + separator.size - 1
      end
    else
      line_ending = @string.length - 1
    end

    if limit && line_ending - @index + 1 > limit
      line_ending = @index + limit - 1
    end

    @string[@index..line_ending].tap do |result|
      if chomp
        @index = line_ending + separator.size + 1

        # I don't really understand why ruby does this but it seems correct?
        if result[-1] == "\r"
          result.replace(result[0..-2])
        end
      else
        @index = line_ending + 1
      end
      @lineno += 1

      # If "" is given as a separator we want to read each paragraph.
      # This means the separator is set to \n\n, but if there are more
      # line ending we want to skip over those.
      # See spec/library/stringio/shared/each.rb:43
      if result && initial_separator == ''
        while @string[@index] == "\n"
          @index += 1
        end
      end
    end
  end
end
