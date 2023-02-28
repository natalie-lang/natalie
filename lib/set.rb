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

  def ^(other)
    unless other.is_a?(Enumerable)
      raise ArgumentError, 'value must be enumerable'
    end

    (self | other) - (self & other)
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

  def collect!(&block)
    replace(each, &block)
  end
  alias map! collect!

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

  def delete_if
    return enum_for(:delete_if) unless block_given?
    each do |element|
      @data.delete(element) if yield element
    end
  end

  def difference(other)
    unless other.is_a?(Enumerable)
      raise ArgumentError, 'value must be enumerable'
    end

    self.class.new(to_a - other.to_a)
  end
  alias - difference

  def disjoint?(other)
    !intersect?(other)
  end

  def empty?
    @data.empty?
  end

  def eql?(other)
    self.class == other.class && @data == other.instance_variable_get(:@data)
  end

  def include?(obj)
    @data.key?(obj)
  end
  alias member? include?
  alias === include?

  def inspect
    items = to_a.map { |e| equal?(e) ? '#<Set: {...}>' : e.to_s }
    "#<Set: {#{items.join(', ')}}>"
  end
  alias to_s inspect

  def intersect?(other)
    unless other.is_a?(Enumerable)
      raise ArgumentError, 'value must be enumerable'
    end
    other.any? { |obj| include?(obj) }
  end

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
      @data.keys.each
    end
  end

  def merge(other)
    unless other.is_a?(Enumerable)
      raise ArgumentError, "value must be enumerable"
    end
    other.each { |element| add(element) }
    self
  end

  def replace(other, &block)
    clear
    other.each do |element|
      if block
        add(block.call(element))
      else
        add(element)
      end
    end
    self
  end

  def union(other)
    unless other.is_a?(Enumerable)
      raise ArgumentError, 'value must be enumerable'
    end

    self.class.new(to_a + other.to_a)
  end
  alias + union
  alias | union

  def select!
    return enum_for(:select!) { size } unless block_given?
    raise FrozenError, "can't modify frozen #{self.class.name}: #{inspect}" if frozen?

    modified = false
    each do |key, value|
      unless yield(key, value)
        delete(key)
        modified = true
      end
    end

    modified ? self : nil
  end
  alias filter! select!

  def reject!
    return enum_for(:reject!) { size } unless block_given?
    raise FrozenError, "can't modify frozen #{self.class.name}: #{inspect}" if frozen?

    modified = false
    each do |value|
      if yield(value)
        delete(value)
        modified = true
      end
    end

    modified ? self : nil
  end

  def subset?(other)
    unless other.is_a?(self.class)
      raise ArgumentError, 'value must be a set'
    end
    all? { |element| other.include?(element) }
  end
  alias <= subset?
end

module Enumerable
  def to_set(&block)
    if block
      Set.new(map(&block))
    else
      Set.new(self)
    end
  end
end
