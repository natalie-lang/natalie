# ARGF is the virtual concatenation of the files named in ARGV (or $stdin when
# ARGV is empty). Its class is anonymous; it is accessible only via
# ARGF.class, matching MRI's behaviour.

argf_class = Class.new do
  include Enumerable

  def initialize(*argv)
    @argv = argv
  end

  attr_reader :argv

  def to_s
    'ARGF'
  end
  alias_method :inspect, :to_s

  def file
    advance!
    @current_file
  end

  def filename
    advance!
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

  private

  def advance!
    return if @current_file
    if argv.empty?
      @current_filename = '-'
      @current_file = $stdin
    else
      @current_filename = argv.shift
      @current_file = @current_filename == '-' ? $stdin : File.open(@current_filename, 'r')
    end
    $FILENAME = @current_filename
  end
end

# The primordial ARGF's argv is always the ARGV global. This file is loaded
# during build_top_env(), which runs before main() populates ARGV, so we
# resolve the constant lazily on each call rather than capturing it now.
ARGF = argf_class.new
def ARGF.argv
  ARGV
end
