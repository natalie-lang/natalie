require 'natalie/inline'

module Math
  class DomainError < StandardError
  end

  class << self
    __function__('::acos', ['double'], 'double')

    def acos(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      raise DomainError, 'Numerical argument is out of domain' unless x.between?(-1.0, 1.0)
      __call__('::acos', x)
    end

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

  def acos(x)
    Math.acos(x)
  end

  def hypot(x, y)
    Math.hypot(x, y)
  end

  def sqrt(x)
    Math.sqrt(x)
  end
end
