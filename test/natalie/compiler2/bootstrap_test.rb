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

  def fail
    raise 'test failed'
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
    assert_eq([1, [2, 3], 4, 5], splat_middle(1, 2, 3, 4, 5))
    assert_eq([1, [2, 3]], splat_right(1, 2, 3))
  end

  def test_destructure_args
    assert_eq([1, 2, 3, 4], destructure_left([[1, 2, :ignored], 3, :ignored], 4))
    assert_eq([1, 2, 3, 4, 5, 6], destructure_middle(1, [2, [3, 4, :ignored], 5], 6))
    assert_eq([1, 2, 3, 4], destructure_right(1, [2, [3, 4, :ignored], :ignored]))
  end

  def test_optional_args
    assert_eq([1, 2], optional_left(1, 2))
    assert_eq([nil, 2], optional_left(nil, 2))
    assert_eq([:default, 1], optional_left(1))
    assert_eq([1, 2, 3], optional_middle(1, 2, 3))
    assert_eq([1, nil, 3], optional_middle(1, nil, 3))
    assert_eq([1, :default, 2], optional_middle(1, 2))
    assert_eq([1, 2], optional_right(1, 2))
    assert_eq([1, nil], optional_right(1, nil))
    assert_eq([1, :default], optional_right(1))
  end

  def test_and
    assert_eq(true && true, true)
    assert_eq(true && false, false)
    assert_eq(true && nil, nil)
    assert_eq(false && nil, false)
    assert_eq(nil && false, nil)
    false && fail
  end

  def test_or
    assert_eq(false || true, true)
    assert_eq(false || false, false)
    assert_eq(false || nil, nil)
    assert_eq(true || nil, true)
    assert_eq(false || 1337, 1337)
    assert_eq(nil || :foo, :foo)
    true || fail
  end

  private

  def ary
    [1, 2, 3]
  end

  def splat_left(*a, b)
    [a, b]
  end

  def splat_middle(a, *b, c, d)
    [a, b, c, d]
  end

  def splat_right(a, *b)
    [a, b]
  end

  def destructure_left(((a, b), c), d)
    [a, b, c, d]
  end

  def destructure_middle(a, (b, (c, d), e), f)
    [a, b, c, d, e, f]
  end

  def destructure_right(a, (b, (c, d)))
    [a, b, c, d]
  end

  def optional_left(x = :default, y)
    [x, y]
  end

  def optional_middle(x, y = :default, z)
    [x, y, z]
  end

  def optional_right(x, y = :default)
    [x, y]
  end
end

TestCompiler2.new.run
