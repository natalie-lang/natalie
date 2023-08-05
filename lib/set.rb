class Set
  include Enumerable

  def initialize(items = nil)
    @data = Hash.new

    return if items.nil?

    if items.respond_to?(:each_entry)
      if block_given?
        items.each_entry { |item| add(yield item) }
      else
        items.each_entry { |item| add(item) }
      end
    elsif items.respond_to?(:each)
      if block_given?
        items.each { |item| add(yield item) }
      else
        items.each { |item| add(item) }
      end
    else
      raise ArgumentError, 'value must be enumerable'
    end
  end

  def initialize_clone(other, freeze: nil)
    super
    @data = other.instance_variable_get(:@data).clone(freeze: freeze)
  end

  def self.[](*items)
    new(items)
  end

  def <=>(other)
    return unless other.is_a?(self.class)

    if self == other
      return 0
    elsif proper_subset?(other)
      return -1
    elsif proper_superset?(other)
      return 1
    end
  end

  def ==(other)
    if eql?(other)
      return true
    end
    if other.class == self.class
      return @data == other.instance_variable_get(:@data)
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

  def compare_by_identity
    @data.compare_by_identity
    self
  end

  def compare_by_identity?
    @data.compare_by_identity?
  end

  def freeze
    @data.freeze
    super
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

  def flatten_merge(other, ids = Set.new)
    other.each do |obj|
      if obj.is_a?(self.class)
        if ids.include?(obj.object_id)
          raise ArgumentError, 'tried to flatten recursive Set'
        end
        ids.add(obj.object_id)
        flatten_merge(obj, ids)
        ids.delete(obj.object_id)
      else
        add(obj)
      end
    end
    self
  end
  protected :flatten_merge

  def flatten
    self.class.new.flatten_merge(self)
  end

  def flatten!
    if any? { |obj| obj.is_a?(self.class) }
      flattened = flatten
      clear
      flattened.each { |obj| add(obj) }
      self
    end
  end

  def hash
    @data.hash
  end

  def include?(obj)
    @data.key?(obj)
  end
  alias member? include?
  alias === include?

  def inspect
    items = to_a.map { |e| equal?(e) ? '#<Set: {...}>' : e.inspect }
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

  def keep_if
    return enum_for(:keep_if) { size } unless block_given?
    each do |element|
      @data.delete(element) unless yield element
    end
    self
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

  def proper_subset?(other)
    unless other.is_a?(self.class)
      raise ArgumentError, 'value must be a set'
    end
    size < other.size && all? { |obj| other.include?(obj) }
  end
  alias < proper_subset?

  def proper_superset?(other)
    unless other.is_a?(self.class)
      raise ArgumentError, 'value must be a set'
    end
    other.size < size && other.all? { |obj| include?(obj) }
  end
  alias > proper_superset?

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

  def subtract(enum)
    enum.each { |obj| delete(obj) }
    self
  end

  def superset?(other)
    unless other.is_a?(self.class)
      raise ArgumentError, 'value must be a set'
    end
    other.all? { |element| include?(element) }
  end
  alias >= superset?
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
