class Range
  def count(*args, &block)
    if args.empty? && block.nil?
      return Float::INFINITY if self.begin.nil?
      return Float::INFINITY if self.end.nil?
    end
    super(*args, &block)
  end

  def size
    return Float::INFINITY if self.begin.nil?
    return unless self.begin.is_a?(Numeric)
    return Float::INFINITY if self.end.nil?
    return unless self.end.is_a?(Numeric)
    return 0 if self.end < self.begin
    return Float::INFINITY if self.begin.is_a?(Float) && self.begin.infinite?
    return Float::INFINITY if self.end.is_a?(Float) && self.end.infinite?
    size = self.end - self.begin
    size += 1 unless exclude_end?
    return size.floor if size.is_a?(Float)
    size
  end
end
