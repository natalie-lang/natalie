class Set
  def initialize
    @data = Hash.new
  end

  def add(obj)
    @data[obj] = true
    self
  end

  def add?(obj)
    if include?(obj)
      nil
    else
      @data[obj] = true
      self
    end
  end

  def include?(obj)
    @data.key?(obj)
  end
end
