autoload :Set, 'set'

module Enumerable
  def to_set(klass = Set, &block)
    if block
      klass.new(map(&block))
    else
      klass.new(self)
    end
  end
end
