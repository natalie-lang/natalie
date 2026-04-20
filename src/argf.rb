# ARGF is the virtual concatenation of the files named in ARGV (or $stdin when
# ARGV is empty). Its class is anonymous; it is accessible only via ARGF.class.

argf_class = Class.new do
  include Enumerable

  def initialize(*argv)
    @argv = argv
    @lineno = 0
  end

  attr_reader :argv, :lineno

  def lineno=(value)
    @lineno = value.to_int
    $. = @lineno
  end

  def to_s
    'ARGF'
  end
  alias_method :inspect, :to_s

  def file
    advance! unless @current_file
    @current_file
  end

  def filename
    advance! unless @current_file
    @current_filename
  end
  alias_method :path, :filename

  def to_io
    file
  end

  def closed?
    return false unless @current_file
    @current_file.closed?
  end

  def close
    return self if @current_file.nil? || @current_file == $stdin || @current_file.closed?
    @current_file.close
    self
  end

  def gets(*args)
    advance! unless @current_file
    loop do
      line = @current_file.gets(*args)
      if line
        @lineno += 1
        $. = @lineno
        $_ = line
        return line
      end
      return nil unless advance!
    end
  end

  def readline(*args)
    line = gets(*args)
    raise EOFError, 'end of file reached' unless line
    line
  end

  def readlines(*args)
    lines = []
    while (line = gets(*args))
      lines << line
    end
    lines
  end
  alias_method :to_a, :readlines

  def each_line(*args)
    return to_enum(:each_line, *args) unless block_given?
    while (line = gets(*args))
      yield line
    end
    self
  end
  alias_method :each, :each_line

  def getc
    advance! unless @current_file
    loop do
      c = @current_file.getc
      return c if c
      return nil unless advance!
    end
  end

  def readchar
    c = getc
    raise EOFError, 'end of file reached' unless c
    c
  end

  def getbyte
    advance! unless @current_file
    loop do
      b = @current_file.getbyte
      return b if b
      return nil unless advance!
    end
  end

  def readbyte
    b = getbyte
    raise EOFError, 'end of file reached' unless b
    b
  end

  def read(length = nil, outbuf = nil)
    outbuf = outbuf ? outbuf.clear : +''
    advance! unless @current_file
    if length.nil?
      loop do
        chunk = @current_file.read
        outbuf << chunk if chunk
        break unless advance!
      end
      @done = true
      outbuf
    else
      remaining = length
      while remaining > 0
        chunk = @current_file.read(remaining)
        if chunk.nil? || chunk.empty?
          break unless advance!
        else
          outbuf << chunk
          remaining -= chunk.bytesize
        end
      end
      length.zero? || !outbuf.empty? ? outbuf : nil
    end
  end

  def eof?
    raise IOError, 'stream already closed' if @done
    advance! unless @current_file
    @current_file.eof?
  end
  alias_method :eof, :eof?

  def each_char
    return to_enum(:each_char) unless block_given?
    while (c = getc)
      yield c
    end
    self
  end
  alias_method :chars, :each_char

  def each_byte
    return to_enum(:each_byte) unless block_given?
    while (b = getbyte)
      yield b
    end
    self
  end
  alias_method :bytes, :each_byte

  def each_codepoint
    return to_enum(:each_codepoint) unless block_given?
    each_char { |c| yield c.ord }
    self
  end
  alias_method :codepoints, :each_codepoint

  private

  def advance!
    if @current_file
      return false if argv.empty?
      @current_file.close unless @current_file == $stdin || @current_file.closed?
    end
    if argv.empty?
      @current_filename = '-'
      @current_file = $stdin
    else
      @current_filename = argv.shift
      @current_file = @current_filename == '-' ? $stdin : File.open(@current_filename, 'r')
    end
    $FILENAME = @current_filename
    true
  end
end

# The primordial ARGF's argv is always the ARGV global. This file is loaded
# during build_top_env(), which runs before main() populates ARGV, so we
# resolve the constant lazily on each call rather than capturing it now.
ARGF = argf_class.new
def ARGF.argv
  ARGV
end
