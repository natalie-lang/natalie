class Hash
  class << self
    alias allocate new

    def try_convert(obj)
      if obj.is_a? Hash
        obj
      elsif obj.respond_to?(:to_hash)
        obj.to_hash.tap do |hash|
          unless hash.nil? || hash.is_a?(Hash)
            raise TypeError, "can't convert #{obj.class.inspect} to Hash (#{obj.class.inspect}#to_hash gives #{hash.class.inspect})"
          end
        end
      end
    end
  end

  def assoc(arg)
    each { |key, value| return key, value if arg == key }
    nil
  end

  def deconstruct_keys(keys)
    self
  end

  def key(value)
    each { |k, v| return k if v == value }
    nil
  end

  alias index key

  def each_key
    return enum_for(:each_key) { size } unless block_given?

    each { |key, _| yield key }
  end

  def each_value
    return enum_for(:each_value) { size } unless block_given?

    each { |_, value| yield value }
  end

  def flatten(level = 1)
    to_a.flatten(level)
  end

  def invert
    new_hash = {}
    each { |key, value| new_hash[value] = key }
    new_hash
  end

  def rassoc(arg)
    each { |key, value| return key, value if arg == value }
    nil
  end

  def reject(&block)
    return enum_for(:reject) { size } unless block_given?

    dup.tap { |new_hash| new_hash.reject!(&block) }
  end

  def reject!(&block)
    return enum_for(:reject!) { size } unless block_given?

    raise FrozenError, "can't modify frozen #{self.class.name}: #{inspect}" if frozen?

    modified = false
    each do |key, value|
      if block.call(key, value)
        delete(key)
        modified = true
      end
    end

    modified ? self : nil
  end

  def select(&block)
    return enum_for(:select) { size } unless block_given?

    dup.tap { |new_hash| new_hash.select!(&block) }
  end
  alias filter select

  def select!(&block)
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

  def shift
    raise FrozenError, "can't modify frozen #{self.class.name}: #{inspect}" if frozen?

    return nil if empty?

    first.tap { |key, _| delete(key) }
  end

  def transform_keys(hash2 = nil, &block)
    return enum_for(:transform_keys) { size } if hash2.nil? && !block_given?

    new_hash = {}
    unless empty?
      each do |key, value|
        new_key = if !hash2.nil? && hash2.key?(key)
          hash2[key]
        elsif block
          block.call(key)
        else
          key
        end
        new_hash[new_key] = value
      end
    end
    new_hash
  end

  def transform_keys!(hash2 = nil, &block)
    return enum_for(:transform_keys!) { size } if hash2.nil? && !block_given?

    raise FrozenError, "can't modify frozen #{self.class.name}: #{inspect}" if frozen?

    unless empty?
      new_keys = Hash.new(0)
      duped = dup
      duped.each do |key, value|
        new_key = if !hash2.nil? && hash2.key?(key)
          hash2[key]
        elsif block
          block.call(key)
        else
          key
        end
        self.delete(key) if new_keys[key]
        self[new_key] = value
        new_keys[new_key] = nil
      end
      new_keys.clear
    end
    self
  end

  def transform_values
    return enum_for(:transform_values) { size } unless block_given?

    new_hash = {}
    each { |key, value| new_hash[key] = yield(value) }
    new_hash
  end

  def transform_values!
    return enum_for(:transform_values!) { size } unless block_given?

    raise FrozenError, "can't modify frozen #{self.class.name}: #{inspect}" if frozen?

    each { |key, value| self[key] = yield(value) }
  end

  def to_proc
    lambda { |arg| self[*arg] }
  end

  def values_at(*keys)
    [].tap { |values| keys.each { |key| values << self[key] } }
  end
end
