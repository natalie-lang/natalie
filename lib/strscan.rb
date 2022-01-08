class StringScanner
  class Error < StandardError
  end

  private def initialize(string)
    @string = string.to_str
    @pos = 0
    @prev_pos = nil
    @matched = nil
  end

  attr_reader :string, :matched, :pos

  def inspect
    if @pos >= @string.size
      '#<StringScanner fin>'
    else
      before = @pos == 0 ? nil : "#{@string[0...@pos].inspect} "
      after = " #{@string[@pos...@pos + 5].inspect[0..-2]}...\""
      "#<StringScanner #{@pos}/#{@string.size} #{before}@#{after}>"
    end
  end

  def pos=(index)
    index = @string.size + index if index < 0
    raise RangeError, 'pos negative' if index < 0
    raise RangeError, 'pos too far' if index > @string.size
    @pos = index
  end

  def string=(s)
    @string = s.to_str
    @pos = 0
  end

  def pointer
    pos
  end

  def pointer=(index)
    self.pos = index
  end

  def eos?
    @pos >= @string.size
  end

  alias empty? eos?

  def check(pattern)
    anchored_pattern = Regexp.new('^' + pattern.source, pattern.options)
    if (@match = rest.match(anchored_pattern))
      @matched = @match.to_s
    else
      @matched = nil
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

  def scan(pattern)
    anchored_pattern = Regexp.new('^' + pattern.source, pattern.options)
    if (@match = rest.match(anchored_pattern))
      @matched = @match.to_s
      @prev_pos = @pos
      @pos += @matched.size
      @matched
    else
      @matched = nil
    end
  end

  def scan_until(pattern)
    start = @pos
    until (matched = scan(pattern))
      return nil if @pos > @string.size
      @pos += 1
    end
    @string[start...@pos + matched.size - 1]
  end

  def skip(pattern)
    anchored_pattern = Regexp.new('^' + pattern.source, pattern.options)
    if (match = rest.match(anchored_pattern))
      matched = match.to_s
      @pos += matched.size
      matched.size
    end
  end

  def skip_until(pattern)
    start = @pos
    until (matched = scan(pattern))
      return nil if @pos > @string.size
      @pos += 1
    end
    @pos - start
  end

  def match?(pattern)
    anchored_pattern = Regexp.new('^' + pattern.source, pattern.options)
    @prev_pos = @pos
    if (@match = rest.match(anchored_pattern))
      @matched = @match.to_s
      @matched.size
    else
      @matched = nil
    end
  end

  def unscan
    if @match
      @pos = @prev_pos
      @match = nil
      @matched = nil
    else
      raise ScanError, 'no previous match'
    end
  end

  def peek(length)
    raise ArgumentError, 'length is negative' if length < 0
    @string.bytes[@pos...(@pos + length)].map(&:chr).join
  end

  alias peep peek

  def scan_full(pattern, advance_pointer_p, return_string_p)
    start = @pos
    scan(pattern)
    distance = @pos - start
    @pos = start unless advance_pointer_p
    return_string_p ? @matched : distance
  end

  alias search_full scan_full

  def get_byte
    @matched = scan(/./)
  end

  def getch
    c = @string.chars[@pos]
    @prev_pos = @pos
    @pos += 1
    @matched = c
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
    @string[0...@prev_pos] if @prev_pos
  end

  def post_match
    return nil if @prev_pos.nil?
    @string[@pos..-1]
  end

  def matched?
    !!@matched
  end

  def matched_size
    @matched ? @matched.size : nil
  end

  def <<(str)
    raise TypeError, 'cannot convert argument to string' unless str.is_a?(String)
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

  def rest_size
    rest.size
  end

  alias restsize rest_size

  def rest?
    @pos < @string.size
  end

  def reset
    @pos = 0
    @match = nil
    @matched = nil
  end

  def terminate
    @pos = @string.size
  end

  alias clear terminate

  def self.must_C_version
    self
  end
end

ScanError = StringScanner::Error
