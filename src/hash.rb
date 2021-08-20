class Hash
  def key(value)
    each do |k, v|
      return k if v == value
    end
    nil
  end

  alias index key

  def transform_values
    return enum_for(:transform_values) { size } unless block_given?

    new_hash = {}
    each do |key, value|
      new_hash[key] = yield(value)
    end
    new_hash
  end
end
