autoload :Set, 'set'

module Enumerable
  def to_set(&block)
    if block
      Set.new(map(&block))
    else
      Set.new(self)
    end
  end
end
