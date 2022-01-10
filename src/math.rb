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

    __function__('::acosh', ['double'], 'double')

    def acosh(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      raise DomainError, 'Numerical argument is out of domain' if x < 1.0
      __call__('::acosh', x)
    end

    __function__('::asin', ['double'], 'double')

    def asin(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      raise DomainError, 'Numerical argument is out of domain' unless x.between?(-1.0, 1.0)
      __call__('::asin', x)
    end

    __function__('::asinh', ['double'], 'double')

    def asinh(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      __call__('::asinh', x)
    end

    __function__('::atan2', %w[double double], 'double')

    def atan2(x, y)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      begin
        y = Float(y)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      __call__('::atan2', x, y)
    end

    __function__('::atan', ['double'], 'double')

    def atan(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      __call__('::atan', x)
    end

    __function__('::atanh', ['double'], 'double')

    def atanh(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      raise DomainError, 'Numerical argument is out of domain' unless x.between?(-1.0, 1.0)
      __call__('::atanh', x)
    end

    __function__('::cbrt', ['double'], 'double')

    def cbrt(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      __call__('::cbrt', x)
    end

    __function__('::cos', ['double'], 'double')

    def cos(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      __call__('::cos', x)
    end

    __function__('::cosh', ['double'], 'double')

    def cosh(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      __call__('::cosh', x)
    end

    __function__('::erf', ['double'], 'double')

    def erf(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      __call__('::erf', x)
    end

    __function__('::erfc', ['double'], 'double')

    def erfc(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      __call__('::erfc', x)
    end

    __function__('::exp', ['double'], 'double')

    def exp(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      __call__('::exp', x)
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

  def acosh(x)
    Math.acosh(x)
  end

  def asin(x)
    Math.asin(x)
  end

  def asinh(x)
    Math.asinh(x)
  end

  def atan2(x, y)
    Math.atan2(x, y)
  end

  def atan(x)
    Math.atan(x)
  end

  def atanh(x)
    Math.atanh(x)
  end

  def cbrt(x)
    Math.cbrt(x)
  end

  def cos(x)
    Math.cos(x)
  end

  def cosh(x)
    Math.cosh(x)
  end

  def erf(x)
    Math.erf(x)
  end

  def erfc(x)
    Math.erfc(x)
  end

  def exp(x)
    Math.exp(x)
  end

  def hypot(x, y)
    Math.hypot(x, y)
  end

  def sqrt(x)
    Math.sqrt(x)
  end
end
