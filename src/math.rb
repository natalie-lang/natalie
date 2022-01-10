require 'natalie/inline'

module Math
  class DomainError < StandardError
  end

  class << self
    def hypot(x, y)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      begin
        y = Float(y)
      rescue ArgumentError
        raise TypeError, "can't convert #{y.class.name} into Float"
      end
      sqrt(x**2 + y**2)
    end

    __function__('::sqrt', ['double'], 'double')

    def sqrt(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      __call__('::sqrt', x)
    end
  end

  private

  def hypot(x, y)
    Math.hypot(x, y)
  end

  def sqrt(x)
    Math.sqrt(x)
  end
end
