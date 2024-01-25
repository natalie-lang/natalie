class Pathname
  TO_PATH = :to_path
  SEPARATOR_PATH = Regexp.new(File::SEPARATOR)
  SEPARATOR_LIST = File::SEPARATOR

  attr_reader :path

  class << self
    def pwd
      new(Dir.getwd)
    end

    alias getwd pwd
  end

  def initialize(value)
    if value.is_a?(Pathname)
      @path = value.path
      return
    elsif value.respond_to?(:to_path)
      value = value.to_path
    end
    raise TypeError, "no implicit conversion of #{value.class} into String" unless value.is_a?(String)
    raise ArgumentError, 'pathname contains null byte' if value.include?("\0")
    @path = value
  end

  def absolute?
    @path.start_with?(File::SEPARATOR)
  end

  def empty?
    return Dir.empty?(@path) if File.directory?(@path)
    File.empty?(@path)
  end

  def equal?(other)
    @path == other.path
  end

  def hash
    @path.hash
  end

  def inspect
    "#<#{self.class}:#{@path}>"
  end

  def join(*args)
    return self if args.empty?
    args.each { |arg| self + arg }
    self
  end

  def parent
    self + '..'
  end

  def relative?
    !absolute?
  end

  def root?
    return false if @path.empty?
    @path.chars.all? { |c| c == File::SEPARATOR }
  end

  def sub(pattern, replace)
    newpath = self.class.new path.sub(pattern, replace)
    newpath.path
  end

  def to_path
    @path
  end

  def to_s
    @path
  end

  def ==(other)
    @path == other.path
  end

  # TODO : handle second-path that starts with combinations of '.' and/or '..'
  def +(other)
    secondpath = Pathname.new(other)
    if secondpath.absolute?
      @path = secondpath.path
    else
      while @path.end_with?(File::SEPARATOR)
        @path.chop!
      end
      @path << File::SEPARATOR
      @path << secondpath.path
    end
    self
  end

  alias / +
end

module Kernel
  private def Pathname(arg) # rubocop:disable Naming/MethodName
    Pathname.new(arg)
  end
  def self.Pathname(arg) # rubocop:disable Naming/MethodName
    Pathname.new(arg)
  end
end
