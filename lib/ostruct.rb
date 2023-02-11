class OpenStruct
  def initialize(args = {})
    @table = {}
    args.each_pair do |key, value|
      key = key.to_sym
      @table[key] = value
      define_singleton_method(key) { @table[key] }
      define_singleton_method("#{key}=") { |value| @table[key] = value }
    end
  end

  def [](key)
    @table[key.to_sym]
  end

  def []=(key, value)
    @table[key.to_sym] = value
    unless respond_to?(key)
      define_singleton_method(key) { @table[key] }
    end
    unless respond_to?("#{key}=")
      define_singleton_method("#{key}=") { |value| @table[key] = value }
    end
  end

  def delete_field(key)
    @table.delete(key.to_sym)
    singleton_class.undef_method(key)
    singleton_class.undef_method("#{key}=")
  end

  def freeze
    @table.freeze
    super
  end

  def inspect
    fields = [self.class] + @table.map do |key, value|
      "#{key}=#{value.equal?(self) ? "#<#{self.class} ...>" : value.inspect}"
    end
    "#<#{fields.join(' ')}>"
  end
  alias to_s inspect

  def method_missing(method, *args)
    if method.to_s[-1] == '='
      if args.size != 1
        raise ArgumentError, "wrong number of arguments (given #{args.size}, expected 1)"
      end

      m = define_singleton_method(method) { |value| @table[method.to_s.chop.to_sym] = value }
      return send(method, *args)
    elsif args.empty?
      define_singleton_method(method) { @table[method.to_sym] }
      return send(method)
    end

    super
  end

  def to_h(&block)
    if block
      @table.to_h(&block)
    else
      @table.dup
    end
  end
end
