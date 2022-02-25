# some basic tests as we are starting out

class TestCase
  def assert_eq(expected, actual)
    if expected != actual
      puts 'expected: ' + expected.inspect
      puts '  actual: ' + actual.inspect
      fail
    end
  end

  def assert_raises(klass, message = nil)
    begin
      yield
    rescue klass => e
      # good
      assert_eq(message, e.message) if message
    else
      puts 'expected to raise: ' + klass.name
      fail
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

  def test_constant
    assert_eq(:toplevel, CONSTANT)
    assert_eq(:toplevel, ::CONSTANT)
    assert_eq(:toplevel2, ::CONSTANT2)
    assert_eq(:namespaced, Constants::CONSTANT)
    assert_eq(:nested, Constants::Nested::CONSTANT)
  end

  def test_float
    f = 1.56
    assert_eq(1.56, f)
  end

  def test_hash
    h = { 1 => 2, 3 => 4 }
    assert_eq(2, h.size)
    assert_eq(2, h[1])
    assert_eq(4, h[3])
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

    x = nil
    if nil
      # do nothing
    else
      x = 'x'
    end
    assert_eq('x', x)
  end

  def test_lambda
    l1 = -> { 1 }
    assert_eq(1, l1.call)
    assert_eq(1, l1.())
    l2 = ->(x) { x }
    assert_eq(2, l2[2])
    l3 = lambda { |x, y| y }
    assert_eq(3, l3.call(2, 3))
  end

  def test_range
    r = 1..3
    assert_eq(1, r.begin)
    assert_eq(3, r.end)
    assert_eq(false, r.exclude_end?)

    r = 1...3
    assert_eq(1, r.begin)
    assert_eq(3, r.end)
    assert_eq(true, r.exclude_end?)

    r = (..3)
    assert_eq(nil, r.begin)
    assert_eq(3, r.end)
    assert_eq(false, r.exclude_end?)

    r = (1...)
    assert_eq(1, r.begin)
    assert_eq(nil, r.end)
    assert_eq(true, r.exclude_end?)
  end

  def test_splat_args
    assert_eq([[1, 2], 3], arg_splat_left(1, 2, 3))
    assert_eq([1, [2, 3], 4, 5], arg_splat_middle(1, 2, 3, 4, 5))
    assert_eq([1, [2, 3]], arg_splat_right(1, 2, 3))
  end

  def test_destructure_args
    assert_eq([1, 2, 3, 4], arg_destructure_left([[1, 2, :ignored], 3, :ignored], 4))
    assert_eq([1, 2, 3, 4, 5, 6], arg_destructure_middle(1, [2, [3, 4, :ignored], 5], 6))
    assert_eq([1, 2, 3, 4], arg_destructure_right(1, [2, [3, 4, :ignored], :ignored]))
  end

  def test_optional_args
    assert_eq([1, 2], arg_optional_left(1, 2))
    assert_eq([nil, 2], arg_optional_left(nil, 2))
    assert_eq([:default, 1], arg_optional_left(1))
    assert_eq([1, 2, 3], arg_optional_middle(1, 2, 3))
    assert_eq([1, nil, 3], arg_optional_middle(1, nil, 3))
    assert_eq([1, :default, 2], arg_optional_middle(1, 2))
    assert_eq([1, 2], arg_optional_right(1, 2))
    assert_eq([1, nil], arg_optional_right(1, nil))
    assert_eq([1, :default], arg_optional_right(1))
  end

  def test_keyword_args
    assert_eq([3, 4], arg_keyword_optional(x: 3, y: 4))
    assert_eq([nil, :default], arg_keyword_optional)
    assert_eq([nil, nil], arg_keyword_optional(x: nil, y: nil))
    assert_eq([1, 2], arg_keyword_optional_after_positional(1, y: 2))
    assert_eq([1, :default], arg_keyword_optional_after_positional(1))
    # TODO: required keyword args
  end

  def test_and
    assert_eq(true, true && true)
    assert_eq(false, true && false)
    assert_eq(nil, true && nil)
    assert_eq(false, false && nil)
    assert_eq(nil, nil && false)
    false && fail
  end

  def test_or
    assert_eq(true, false || true)
    assert_eq(false, false || false)
    assert_eq(nil, false || nil)
    assert_eq(true, true || nil)
    assert_eq(1337, false || 1337)
    assert_eq(:foo, nil || :foo)
    true || fail
  end

  def test_case
    foo = :bar
    case foo
    when :baz
      fail
    when :bar
      foo = :foo
    else
      fail
    end
    assert_eq(foo, :foo)

    case foo
    when :foo, fail
      foo = :bar
    when fail
      # pass
    else
      fail
    end
    assert_eq(foo, :bar)

    case
    when false
      fail
    when nil
      fail
    when 1 + 1 == 2
      foo = :foo
    end
    assert_eq(foo, :foo)

    case_result = case 0;when 1; end
    assert_eq(case_result, nil)
  end

  def test_rescue
    x = begin
          1
        rescue ArgumentError
          2
        end
    assert_eq(1, x)
    y = begin
          send() # missing args
        rescue NoMethodError
          1
        rescue ArgumentError
          2
        end
    assert_eq(2, y)
    z = begin
          method_raises
        rescue
          y + 1
        end
    assert_eq(3, z)
  end

  def test_rescue_get_exception
    begin
      send()
    rescue NoMethodError => e
      fail # should not be here
    rescue ArgumentError => e
      assert_eq(ArgumentError, e.class)
      assert_eq('no method name given', e.message)
    end
  end

  def test_rescue_else
    x = begin
          1
        rescue
          2
        else
          3
        end
    assert_eq(3, x)

    y = begin
          non_existent_method
        rescue
          2
        else
          3
        end
    assert_eq(2, y)

    assert_raises(RuntimeError, 'this is the error') do
      begin
        # noop
      rescue
        raise 'should not be reached'
      else
        raise 'this is the error'
      end
    end
  end

  def test_block_scope
    y = 1
    2.times do
      y += 2
      3.times do
        y += 3
      end
    end
    assert_eq(23, y)
  end

  def test_block_arg_and_pass
    x = 1
    block_call do
      x = 2
    end
    assert_eq(2, x)
    block_pass do
      x = 3
    end
    assert_eq(3, x)
  end

  def test_block_arg_does_not_overwrite_outer_scope_arg
    z = 0
    [1].each do |z|
      assert_eq(1, z)
    end
    assert_eq(0, z)
  end

  def test_break_from_block
    result = block_yield3(1, 2, 3) do
      break 100
    end
    assert_eq(100, result)
  end

  def test_break_from_loop
    result = loop do
      break 200
    end
    assert_eq(200, result)
  end

  def test_break_from_proc
    the_proc = proc do
      break 300
    end
    assert_raises(LocalJumpError, 'break from proc-closure') do
      the_proc.call
    end
  end

  def test_break_from_lambda
    l1 = -> do
      break 400
    end
    assert_eq(400, l1.call)
    l2 = lambda do
      break 500
    end
    assert_eq(500, l2.call)
  end

  def test_yield
    result = block_yield0 { :hi }
    assert_eq(:hi, result)
    result = block_yield2(1, 2) { |x, y| [x, y] }
    assert_eq([1, 2], result)
  end

  def test_method_in_method
    x = 1
    def should_not_see_outer_scope
      assert_raises(NameError) { x }
      x = 2
    end
    assert_eq(2, should_not_see_outer_scope)
    assert_eq(1, x)
  end

  def test_return
    def should_return_1
      return 1
      2
    end
    assert_eq(1, should_return_1)

    def should_return_ary
      return 1, 2
      3
    end
    assert_eq([1, 2], should_return_ary)

    def should_return_nil
      return
      3
    end
    assert_eq(nil, should_return_nil)
  end

  def test_multiple_assignment
    a, b = [1, 2]
    assert_eq(1, a)
    assert_eq(2, b)
    ((a, b), c) = [[1, 2], 3]
    assert_eq(1, a)
    assert_eq(2, b)
    assert_eq(3, c)
    ((a, *b), c) = [[1, 2, 3], 3]
    assert_eq([2, 3], b)
    ary = [1, 2, 3]
    a, *@b = ary
    assert_eq([2, 3], @b)
    a, *$b = ary
    assert_eq([2, 3], $b)
    def returns_whole_thing
      d, e = [4, 5]
    end
    assert_eq([4, 5], returns_whole_thing)
  end

  def test_global_variable
    $global = 1
    assert_eq(1, $global)
    assert_eq(nil, $non_existent_global)
  end

  def test_instance_variable
    @ivar = 2
    assert_eq(2, @ivar)
    assert_eq(nil, @non_existent_ivar)
  end

  def test_send
    assert_eq('pub', ClassWithPrivateMethod.new.pub)
    assert_raises(NoMethodError) { ClassWithPrivateMethod.new.priv }
  end

  def test_string
    s1 = 'hello world'
    assert_eq(11, s1.size)
    s2 = 'hello ' + 'world'
    assert_eq(s1, s2)
    s3 = 'world'
    s4 = "hello #{s3}"
    assert_eq(s1, s4)
    s5 = "hello #{:world}"
    assert_eq(s1, s5)
    s6 = "#{1 + 2} = 3"
    assert_eq("3 = 3", s6)
  end

  private

  def ary
    [1, 2, 3]
  end

  def arg_splat_left(*a, b)
    [a, b]
  end

  def arg_splat_middle(a, *b, c, d)
    [a, b, c, d]
  end

  def arg_splat_right(a, *b)
    [a, b]
  end

  def arg_destructure_left(((a, b), c), d)
    [a, b, c, d]
  end

  def arg_destructure_middle(a, (b, (c, d), e), f)
    [a, b, c, d, e, f]
  end

  def arg_destructure_right(a, (b, (c, d)))
    [a, b, c, d]
  end

  def arg_optional_left(x = :default, y)
    [x, y]
  end

  def arg_optional_middle(x, y = :default, z)
    [x, y, z]
  end

  def arg_optional_right(x, y = :default)
    [x, y]
  end

  def arg_keyword_optional(x: nil, y: :default)
    [x, y]
  end

  def arg_keyword_optional_after_positional(x, y: :default)
    [x, y]
  end

  def block_yield0
    yield
  end

  def block_yield2(a, b)
    yield a, b
  end

  def block_yield3(a, b, c, &block)
    block_yield2(a, b, &block)
    c
  end

  def block_call(&block)
    block.call
  end

  def block_pass(&block)
    block_call(&block)
  end

  def method_raises
    raise 'foo'
  end

  class ClassWithPrivateMethod
    def pub; 'pub'; end
    private
    def priv; 'priv'; end
  end

  ::CONSTANT = :toplevel

  class Constants
    CONSTANT = :namespaced
  end

  class Constants::Nested
    CONSTANT = :nested
  end
end

CONSTANT2 = :toplevel2

TestCompiler2.new.run
