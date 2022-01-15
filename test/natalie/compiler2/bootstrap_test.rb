# some basic tests as we are starting out

class TestCase
  def assert_eq(expected, actual)
    if expected != actual
      print 'expected: '
      p expected
      print 'actual: '
      p actual
      raise 'test failed'
    end
  end
end

class TestCompiler2 < TestCase
  def test_array
    result = ary.map { |i| i * 2 }
    assert_eq([2, 4, 6], result)
  end

  def test_float
    f = 1.56
    assert_eq(1.56, f)
  end

  def run
    test_array
    test_float
  end

  private

  def ary
    [1, 2, 3]
  end
end

TestCompiler2.new.run
