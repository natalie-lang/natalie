class Set
  include Enumerable

  def initialize(items = nil)
    @data = Hash.new
    
    return if items.nil?

    items.each do |item|
      add(item)
    end
  end

  def self.[](*items)
    new(items)
  end

  def ==(other)
    if eql?(other)
      return true
    end
    if other.is_a?(Set) && self.size == other.size
      return other.all? { |element| @data.include?(element) }
    end
    false
  end

  def length
    @data.length
  end
  alias size length

  def add(obj)
    @data[obj] = true
    self
  end
  alias << add

  def add?(obj)
    if include?(obj)
      nil
    else
      @data[obj] = true
      self
    end
  end

  def classify
    return enum_for(:classify) unless block_given?
    hash = Hash.new
    each do |element|
      block_result = yield element
      hash[block_result] = Set.new unless hash.key?(block_result)
      hash[block_result] << element
    end
    hash
  end

  def delete(obj)
    if include?(obj)
      @data.delete(obj)
    end
    self
  end

  def delete?(obj)
    if include?(obj)
      @data.delete(obj)
      self
    else
      nil
    end
  end

  def eql?(other)
    self.class == other.class && @data == other.instance_variable_get(:@data)
  end

  def include?(obj)
    @data.key?(obj)
  end
  alias === include?

  def inspect
    "#<Set: {#{to_a}}>"
  end
  alias to_s inspect

  def intersection(other)
    unless other.is_a?(Enumerable)
      raise ArgumentError, 'value must be enumerable'
    end

    self.class.new(other.select { |obj| include?(obj) })
  end
  alias & intersection

  def clear
    @data.clear
    self
  end

  def join(sep = nil)
    to_a.join(sep)
  end

  def to_a
    @data.keys
  end

  def each(&block)
    if block
      @data.keys.each(&block)
      self
    else
      @data.keys.each(&block)
    end
  end

  def union(other)
    unless other.is_a?(Enumerable)
      raise ArgumentError, 'value must be enumerable'
    end

    self.class.new(to_a + other.to_a)
  end
  alias + union
  alias | union
end
