# frozen_string_literal: true

class StringIO
  VERSION = '3.1.2'

  include Enumerable

  attr_reader :string
  attr_accessor :lineno

  def self.open(*args, **kwargs)
    stringio = new(*args, **kwargs)
    return stringio unless block_given?

    begin
      yield(stringio)
    ensure
      stringio.instance_variable_set(:@string, nil)
      stringio.close
    end
  end

  private def initialize(string = nil, arg_mode = nil, mode: nil, binmode: nil, textmode: nil)
    if string.nil?
      string = ''.dup.force_encoding(Encoding.default_external)
    elsif !string.is_a?(String)
      string = string.to_str
    end
    @string = string
    @string.force_encoding(Encoding::ASCII_8BIT) if binmode
    @index = 0
    @lineno = 0

    mode ||= arg_mode
    unless mode
      if string.frozen?
        mode = 'r'
      else
        mode = 'r+'
      end
    end

    if mode.is_a?(Integer)
      if (mode & IO::TRUNC) == IO::TRUNC
        @string.clear
        mode &= ~IO::TRUNC
      end

      if (mode & IO::APPEND) == IO::APPEND
        @index = string.size - 1
        mode &= ~IO::APPEND
      end

      case mode
      when IO::RDONLY
        mode = 'r'
      when IO::WRONLY
        mode = 'w'
      when IO::RDWR
        mode = 'r+'
      end
    end

    mode = mode.to_str unless mode.is_a? String
    @mode = mode

    if !binmode.nil?
      if @mode.include?('b')
        raise ArgumentError, 'binmode specified twice'
      elsif @mode.include?('t') || (binmode && textmode)
        raise ArgumentError, 'both textmode and binmode specified'
      end
    elsif !textmode.nil?
      if @mode.include?('t')
        raise ArgumentError, 'textmode specified twice'
      elsif @mode.include?('b')
        raise ArgumentError, 'both textmode and binmode specified'
      end
    end

    __set_closed

    raise Errno::EACCES, 'Permission denied' if !closed_write? && string.frozen?

    @string.clear if @mode == 'w'

    @mutex = Mutex.new

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

    yield __next_line(separator, limit, chomp: chomp) until eof?

    self
  end
  alias each_line each

  def each_byte
    return enum_for(:each_byte) unless block_given?
    __assert_not_read_closed

    getc.each_byte { |b| yield b } until eof?

    self
  end

  def each_char
    return enum_for(:each_char) unless block_given?
    __assert_not_read_closed

    yield getc until eof?

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

    @string[@index].tap { @index += 1 unless eof? }
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
    raise Errno::EINVAL, 'Invalid argument' if new_index < 0

    @index = new_index
  end

  def print(*args)
    args = [$_] if args.empty?
    args.each { |arg| write(arg) }
    write($\) unless $\.nil?
    nil
  end

  def printf(format_str, *args)
    write(Kernel.sprintf(format_str, *args))
    nil
  end

  def puts(*args)
    args = [$_] if args.empty?
    args.flatten.each do |arg|
      arg = arg.to_s unless arg.is_a?(String)
      write(arg)
      write("\n") unless arg.end_with?("\n")
    end
    nil
  end

  def read(length = nil, out_string = nil)
    __assert_not_read_closed

    encoding = nil
    if length
      length = length.to_int if !length.is_a?(Integer) && length.respond_to?(:to_int)

      raise TypeError, "no implicit conversion of #{length.class} into Integer" unless length.is_a? Integer

      raise ArgumentError, "negative length #{length} given" if length < 0

      return +'' if length == 0
      return nil if eof?

      encoding = Encoding::BINARY
    else
      return +'' if eof?

      length = @string.length - @index
    end

    if out_string
      out_string = out_string.to_str if !out_string.is_a?(String) && out_string.respond_to?(:to_str)

      raise TypeError, "no implicit conversion of #{out_string.class} into String" unless out_string.is_a? String
    end

    length = @string.length - @index if @index + length > @string.length

    result = @string[@index..(@index + [length - 1, 0].max)]
    @index += length

    if out_string
      out_string.replace(result)
    else
      result = result.encode(encoding) if encoding
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

  def readbyte
    raise EOFError, 'end of file reached' if eof?
    getbyte
  end

  def readchar
    raise EOFError, 'end of file reached' if eof?
    getc
  end

  def readline(...)
    raise EOFError, 'end of file reached' if eof?
    gets(...)
  end

  def rewind
    @lineno = 0
    @index = 0
  end

  def seek(offset, whence = IO::SEEK_SET)
    raise IOError, 'closed stream' if closed?
    offset = offset.to_int if !offset.is_a?(Integer) && offset.respond_to?(:to_int)
    raise TypeError, "no implicit conversion of #{offset.class} into Integer" unless offset.is_a?(Integer)
    case whence
    when IO::SEEK_CUR
      @index += offset
    when IO::SEEK_END
      @index = @string.size + offset
    when IO::SEEK_SET
      raise Errno::EINVAL, 'Invalid argument' if offset.negative?
      @index = offset
    else
      raise Errno::EINVAL, 'Invalid argument - invalid whence'
    end
  end

  def set_encoding(external_encoding, _ = nil, **_options)
    external_encoding = Encoding.find(external_encoding) if external_encoding.is_a?(String)
    @external_encoding = external_encoding || Encoding.default_external
    @string.force_encoding(@external_encoding) unless @string.frozen?
    self
  end

  def set_encoding_by_bom
    return nil if closed_read?
    raise FrozenError, "can't modify frozen #{self.class}" if frozen?
    if @string.byteslice(0, 4) == "\x00\x00\xFE\xFF".b
      set_encoding(Encoding::UTF_32BE)
    elsif @string.byteslice(0, 4) == "\xFF\xFE\x00\x00".b
      set_encoding(Encoding::UTF_32LE)
    elsif @string.byteslice(0, 2) == "\xFE\xFF".b
      set_encoding(Encoding::UTF_16BE)
    elsif @string.byteslice(0, 2) == "\xFF\xFE".b
      set_encoding(Encoding::UTF_16LE)
    elsif @string.byteslice(0, "\u{FEFF}".bytesize) == "\u{FEFF}".b
      set_encoding(Encoding::UTF_8)
    else
      @index = 0
      return nil
    end
    @index = 1
    external_encoding
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

  def truncate(integer)
    integer = integer.to_int if !integer.is_a?(Integer) && integer.respond_to?(:to_int)
    raise TypeError, "no implicit conversion of #{integer.class} into Integer" unless integer.is_a?(Integer)
    raise Errno::EINVAL, 'Invalid argument - negative length' if integer.negative?
    raise IOError, 'not opened for writing' if closed_write?

    if integer < @string.size
      @string.slice!(integer, @string.size)
    elsif integer > @string.size
      @string << "\x00".b * (integer - @string.size)
    end

    0 # It looks like MRI always returns 0
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

  def ungetc(argument)
    __assert_not_read_closed

    argument = argument.to_str if !argument.is_a?(String) && argument.respond_to?(:to_str)
    raise TypeError, "no implicit conversion of #{argument.class} into String" unless argument.is_a?(String)

    @string.concat("\x00".b * (@index - @string.bytesize)) if @index > @string.bytesize

    if @index.zero?
      @string.prepend(argument)
    else
      argument.bytes.reverse.each do |byte|
        if @index.zero?
          @string.prepend(byte)
        else
          @index -= 1
          @string.setbyte(@index, byte)
        end
      end
    end

    nil
  end

  def putc(argument)
    __assert_not_write_closed

    if !argument.is_a?(String)
      raise TypeError, "no implicit conversion of #{argument.class} into Integer" if !argument.respond_to?(:to_int)

      argument = (argument.to_int % 256).chr
    end

    write(argument.to_s[0])

    argument
  end

  def write(argument)
    __assert_not_write_closed

    argument = argument.to_s unless argument.is_a? String

    @mutex.synchronize do
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

    limit = limit.to_int if limit && !limit.is_a?(Integer)

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

    limit = nil if limit&.negative?

    if limit == 0
      @lineno += 1
      return +''
    end

    separator = "\n\n" if separator == ''

    line_ending = @string[@index..].index(separator) if separator

    if line_ending
      if chomp
        line_ending += @index - 1
      else
        line_ending += @index + separator.size - 1
      end
    else
      line_ending = @string.length - 1
    end

    line_ending = @index + limit - 1 if limit && line_ending - @index + 1 > limit

    @string[@index..line_ending].tap do |result|
      if chomp
        @index = line_ending + separator.size + 1

        # I don't really understand why ruby does this but it seems correct?
        result.replace(result[0..-2]) if result[-1] == "\r"
      else
        @index = line_ending + 1
      end
      @lineno += 1

      # If "" is given as a separator we want to read each paragraph.
      # This means the separator is set to \n\n, but if there are more
      # line ending we want to skip over those.
      # See spec/library/stringio/shared/each.rb:43
      @index += 1 while @string[@index] == "\n" if result && initial_separator == ''
    end
  end
end
