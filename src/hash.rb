class Hash
  class << self
    alias allocate new
  end

  def key(value)
    each do |k, v|
      return k if v == value
    end
    nil
  end

  alias index key

  def transform_keys
    return enum_for(:transform_keys) { size } unless block_given?

    new_hash = {}
    each do |key, value|
      new_hash[yield(key)] = value
    end
    new_hash
  end

  def transform_keys!(&block)
    return enum_for(:transform_keys!) { size } unless block_given?

    raise FrozenError, "can't modify frozen #{self.class.name}: #{inspect}" if frozen?

    # NOTE: have to do it this way due to behavior of break
    duped = dup
    clear
    duped.each do |key, value|
      self[yield(key)] = value
    end
    self
  end

  def transform_values
    return enum_for(:transform_values) { size } unless block_given?

    new_hash = {}
    each do |key, value|
      new_hash[key] = yield(value)
    end
    new_hash
  end
end
