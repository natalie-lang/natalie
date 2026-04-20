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
