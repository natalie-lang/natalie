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
  def ary
    [1, 2, 3]
  end

  def run
    result = ary.map { |i| i * 2 }
    assert_eq([2, 4, 6], result)
  end
end

TestCompiler2.new.run
