class StringIO
  attr_reader :string, :lineno

  private def initialize(string = "", mode = nil)
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
  end

  def close
    @read_closed = true
    @write_closed = true
    nil
  end

  def close_read
    @read_closed = true
    nil
  end

  def closed_read?
    !!@read_closed
  end

  def close_write
    @write_closed = true
    nil
  end

  def closed_write?
    !!@write_closed
  end

  def each(separator = $/, limit = nil, chomp: false)
    return enum_for(:each) unless block_given?
    __assert_not_read_closed

    while !eof?
      yield __next_line(separator, limit, chomp: chomp)
    end

    self
  end
  alias each_line each

  def eof?
    @index >= @string.length
  end
  alias eof eof?

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

  def pos
    @index
  end

  def pos=(new_index)
    if new_index < 0
      raise Errno::EINVAL, 'Invalid argument'
    end

    @index = new_index
  end

  def read(length = nil, out_string = nil)
    __assert_not_read_closed

    encoding = nil
    if length
      if !length.is_a?(Integer) && length.respond_to?(:to_int)
        length = length.to_int
      end

      if !length.is_a? Integer
        raise TypeError, "no implicit conversion of #{length.class} into Integer"
      end

      if length < 0
        raise ArgumentError, "negative length #{length} given"
      end

      return "" if length == 0
      return nil if eof?

      encoding = Encoding::BINARY
    else
      return "" if eof?

      length = @string.length - @index
    end

    if out_string
      if !out_string.is_a?(String) && out_string.respond_to?(:to_str)
        out_string = out_string.to_str
      end

      if !out_string.is_a? String
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

  def write(argument)
    __assert_not_write_closed

    if !argument.is_a? String
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

  private def __assert_not_read_closed
    raise IOError, 'not opened for reading' if closed_read?
  end

  private def __assert_not_write_closed
    raise IOError, 'not opened for reading' if closed_write?
  end

  private def __appending?
    @mode[0] == 'a'
  end

  # FIXME: Tidy this up..
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
      return ""
    end

    if separator == ''
      separator = "\n\n"
    end

    if separator
      line_ending = @string[@index..].index(separator)
    end

    unless line_ending
      line_ending = @string.length - 1
    else
      if chomp
        line_ending += @index - 1
      else
        line_ending += @index + separator.size - 1
      end
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
