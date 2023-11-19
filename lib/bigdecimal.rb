# NATFIXME: This is a stub implementation to help with a few specs

class BigDecimal
  def initialize(str)
    @str = str
  end

  def to_f
    @str.to_f
  end
end

def BigDecimal(str) # rubocop:disable Naming/MethodName
  BigDecimal.new(str) # rubocop:disable Lint/BigDecimalNew
end
