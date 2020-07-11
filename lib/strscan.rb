class StringScanner
  def initialize(string)
    @string = string
    @index = 0
  end

  def eos?
    @index >= @string.size
  end

  def scan(pattern)
    anchored_pattern = Regexp.new('^' + pattern.inspect[1...-1])
    if (match = rest.match(anchored_pattern))
      s = match.to_s
      @index += s.size
      s
    end
  end

  def rest
    @string[@index..-1] || ''
  end
end
