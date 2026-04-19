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
end

# The primordial ARGF cannot capture ARGV here because this file is loaded
# during build_top_env(), which runs before main() populates ARGV. Defer
# that binding until first access.
ARGF = argf_class.allocate
def ARGF.argv
  @argv ||= ARGV
end
