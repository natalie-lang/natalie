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

    __define_method__ :frexp, [:x], <<-END
      KernelModule *kernel = GlobalEnv::the()->Object()->as_kernel_module_for_method_binding();
      FloatObject *value;
      try {
          value = kernel->Float(env, x, nullptr)->as_float();
      } catch (ExceptionObject *exception) {
          ClassObject *klass = exception->klass();
          if (strcmp(klass->inspect_str()->c_str(), "ArgumentError") == 0) {
              env->raise("TypeError", "can't convert {} into Float", x->klass()->inspect_str());
          } else {
              env->raise_exception(exception);
          }
      }
      if (value->is_nan()) {
          return GlobalEnv::the()->Float()->const_get("NAN"_s);
      }
      int exponent;
      auto significand = std::frexp(value->to_double(), &exponent);
      return new ArrayObject { { Value::integer(significand), Value::integer(exponent) } };
    END

    __function__('::tgamma', ['double'], 'double')

    def gamma(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      return x if x == Float::INFINITY
      raise DomainError, 'Numerical argument is out of domain' if x == -Float::INFINITY
      if x == x.floor
        raise DomainError, 'Numerical argument is out of domain' if x < 0
        return FACT_TABLE[x.to_i - 1] if x.between?(1.0, 23.0)
      end
      __call__('::tgamma', x)
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

    __function__('::log10', ['double'], 'double')

    def log10(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      raise DomainError, 'Numerical argument is out of domain' if x < 0
      __call__('::log10', x)
    end

    __function__('::log2', ['double'], 'double')

    def log2(x)
      raise TypeError, "can't convert String into Float" if x.is_a?(String)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      raise DomainError, 'Numerical argument is out of domain' if x < 0
      __call__('::log2', x)
    end

    __function__('::log', ['double'], 'double')

    def log(*args)
      unless args.length.between?(1, 2)
        raise ArgumentError, "wrong number of arguments (given #{args.length}, expected 1..2)"
      end
      x, base = args
      raise TypeError, "can't convert String into Float" if x.is_a?(String)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      raise DomainError, 'Numerical argument is out of domain' if x < 0
      if args.length == 2
        raise TypeError, "can't convert String into Float" if base.is_a?(String)
        begin
          base = Float(base)
        rescue ArgumentError
          raise TypeError, "can't convert #{base.class.name} into Float"
        end
        return __call__('::log', x) / __call__('::log', base)
      end
      __call__('::log', x)
    end

    __function__('::sin', ['double'], 'double')

    def sin(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      __call__('::sin', x)
    end

    __function__('::sinh', ['double'], 'double')

    def sinh(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      __call__('::sinh', x)
    end

    __function__('::tan', ['double'], 'double')

    def tan(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      __call__('::tan', x)
    end

    __function__('::tanh', ['double'], 'double')

    def tanh(x)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      return Float::NAN if x.nan?
      __call__('::tanh', x)
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

  def frexp(x)
    Math.frexp(x)
  end

  def gamma(x)
    Math.gamma(x)
  end

  def hypot(x, y)
    Math.hypot(x, y)
  end

  def log10(x)
    Math.log10(x)
  end

  def log2(x)
    Math.log2(x)
  end

  def log(x)
    Math.log(x)
  end

  def sin(x)
    Math.sin(x)
  end

  def sinh(x)
    Math.sinh(x)
  end

  def tan(x)
    Math.tan(x)
  end

  def tanh(x)
    Math.tanh(x)
  end

  def sqrt(x)
    Math.sqrt(x)
  end

  FACT_TABLE = [
    1.0,
    1.0,
    2.0,
    6.0,
    24.0,
    120.0,
    720.0,
    5040.0,
    40320.0,
    362880.0,
    3628800.0,
    39916800.0,
    479001600.0,
    6227020800.0,
    87178291200.0,
    1307674368000.0,
    20922789888000.0,
    355687428096000.0,
    6402373705728000.0,
    121645100408832000.0,
    2432902008176640000.0,
    51090942171709440000.0,
    1124000727777607680000.0,
  ]
end
