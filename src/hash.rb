class Hash
  class << self
    alias allocate new

    def try_convert(obj)
      if obj.is_a? Hash
        obj
      elsif obj.respond_to?(:to_hash)
        obj.to_hash.tap do |hash|
          unless hash.nil? || hash.is_a?(Hash)
            raise TypeError, "can't convert #{obj.inspect} to Hash (#{obj.inspect}#to_hash gives #{hash.inspect})"
          end
        end
      end
    end
  end

  def assoc(arg)
    each do |key, value|
      if arg == key
        return [key, value]
      end
    end
    nil
  end

  def key(value)
    each do |k, v|
      return k if v == value
    end
    nil
  end

  alias index key

  def each_key
    return enum_for(:each_key) unless block_given?

    each do |key, _|
      yield key
    end
  end

  def each_value
    return enum_for(:each_value) unless block_given?

    each do |_, value|
      yield value
    end
  end

  def has_value?(value)
    !!key(value)
  end
  alias value? has_value?

  def invert
    new_hash = {}
    each do |key, value|
      new_hash[value] = key
    end
    new_hash
  end

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

  def to_proc
    lambda do |arg|
      self[*arg]
    end
  end
end
