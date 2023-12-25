module Warning
  extend self

  FLAGS = {
    deprecated: nil,
    experimental: true,
    performance: false
  }

  private_constant :FLAGS

  def [](category)
    unless category.is_a?(Symbol)
      raise TypeError, "wrong argument type #{category.class.name} (expected Symbol)"
    end

    unless FLAGS.key?(category)
      raise ArgumentError, "unknown category: #{category}"
    end

    flag = FLAGS[category]

    return ($VERBOSE == true) if category == :deprecated && flag.nil?

    flag
  end

  def []=(category, flag)
    unless category.is_a?(Symbol)
      raise TypeError, "wrong argument type #{category.class.name} (expected Symbol)"
    end

    unless FLAGS.key?(category)
      raise ArgumentError, "unknown category: #{category}"
    end

    FLAGS[category] = flag
  end

  def warn(message, category: nil)
    $stderr.print(message) if category.nil? || self[category]

    nil
  end
end
