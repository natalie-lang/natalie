module Warning
  extend self

  FLAGS = { deprecated: nil, experimental: true, performance: false }

  private_constant :FLAGS

  def [](category)
    raise TypeError, "wrong argument type #{category.class.name} (expected Symbol)" unless category.is_a?(Symbol)

    raise ArgumentError, "unknown category: #{category}" unless FLAGS.key?(category)

    flag = FLAGS[category]

    return($VERBOSE == true) if category == :deprecated && flag.nil?

    flag
  end

  def []=(category, flag)
    raise TypeError, "wrong argument type #{category.class.name} (expected Symbol)" unless category.is_a?(Symbol)

    raise ArgumentError, "unknown category: #{category}" unless FLAGS.key?(category)

    FLAGS[category] = flag
  end

  def warn(message, category: nil)
    $stderr.print(message) if category.nil? || self[category]

    nil
  end
end
