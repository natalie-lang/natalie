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

  def run
    tests = methods.select { |m| m.start_with?('test_') }
    tests.each do |method|
      send(method)
    end
    puts tests.size.to_s + ' tests successful'
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

  def test_if
    t = nil
    if 1
      t = 't'
    else
      t = 'f'
    end
    f = nil
    if nil
      f = 't'
    else
      f = 'f'
    end
    assert_eq(%w[t f], [t, f])
  end

  def test_splat_args
    assert_eq([[1, 2], 3], splat_left(1, 2, 3))
    assert_eq([1, [2, 3], 4], splat_middle(1, 2, 3, 4))
    assert_eq([1, [2, 3]], splat_right(1, 2, 3))
  end

  def test_destructure_args
    assert_eq([1, 2, 3, 4], destructure_left([[1, 2], 3], 4))
    assert_eq([1, 2, 3, 4], destructure_left([[1, 2, 3], 3, 4], 4))
  end

  private

  def ary
    [1, 2, 3]
  end

  def splat_left(*a, b)
    [a, b]
  end

  def splat_middle(a, *b, c)
    [a, b, c]
  end

  def splat_right(a, *b)
    [a, b]
  end

  def destructure_left(((a, b), c), d)
    [a, b, c, d]
  end

  def destructure_right(a, (b, (c, d)))
    [a, b, c, d]
  end
end

TestCompiler2.new.run
