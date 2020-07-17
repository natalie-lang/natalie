class StringScanner
  def initialize(string)
    @string = string.to_str
    @pos = 0
    @prev_index = 0
    @matched = nil
  end

  attr_reader :string, :matched
  attr_accessor :pos

  def eos?
    @pos >= @string.size
  end

  alias empty? eos?

  def check(pattern)
    anchored_pattern = Regexp.new('^' + pattern.inspect[1...-1])
    if (@match = rest.match(anchored_pattern))
      @matched = @match.to_s
    else
      @matched = nil
    end
  end

  def scan(pattern)
    anchored_pattern = Regexp.new('^' + pattern.inspect[1...-1])
    if (@match = rest.match(anchored_pattern))
      @matched = @match.to_s
      @prev_index = @pos
      @pos += @matched.size
      @matched
    else
      @matched = nil
    end
  end

  def unscan
    @pos = @prev_index
  end

  def peek(length)
    raise ArgumentError, "length is negative" if length < 0
    @string.bytes[@pos...(@pos+length)].map(&:chr).join
  end

  def get_byte
    scan(/./)
  end

  alias getbyte get_byte

  def [](index)
    return nil unless @match
    if index.is_a?(Integer) || index.is_a?(String)
      return nil unless index.is_a?(Integer) # FIXME
      @match[index]
    else
      raise TypeError, "Bad index: #{index.inspect}"
    end
  end

  def check_until(pattern)
    start = @pos
    until (matched = check(pattern))
      @pos += 1
    end
    @pos += matched.size
    accumulated = @string[start...@pos]
    @pos = start
    accumulated
  end

  def exist?(pattern)
    return 0 if pattern == //
    start = @pos
    while true
      # FIXME: this is ugly because `break` is broken here :-(
      if check(pattern)
        found_at = @pos - start + 1
        @pos = start
        return found_at
      end
      if @pos >= @string.size
        @pos = start
        return nil
      end
      @pos += 1
    end
    @pos = start
    nil
  end

  def pre_match
    @string[0...@prev_index]
  end

  def <<(str)
    unless str.is_a?(String)
      raise TypeError, 'cannot convert argument to string'
    end
    @string << str
    self
  end

  alias concat <<

  def beginning_of_line?
    @pos == 0 || (@pos > 0 && @string[@pos - 1] == "\n")
  end

  alias bol? beginning_of_line?

  def rest
    @string[@pos..-1] || ''
  end

  def rest?
    @pos < @string.size
  end

  def reset
    @pos = 0
  end

  def terminate
    @pos = @string.size
  end

  alias clear terminate
end
