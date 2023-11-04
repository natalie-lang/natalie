class Thread
  def initialize
    raise NoMethodError, 'TODO: Thread.new'
  end

  def self.current
    @current ||= {}
  end
end
