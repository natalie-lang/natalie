# some basic tests as we are starting out

class TestCase
  class TestFailedError < StandardError; end

  def initialize
    @assertions = 0
    @success = 0
    @fail = 0
    @error = 0
  end

  def assert_eq(expected, actual)
    @assertions += 1
    if expected != actual
      puts @test
      puts 'expected: ' + expected.inspect
      puts '  actual: ' + actual.inspect
      fail
    end
  end

  def assert_raises(klass, message = nil)
    @assertions += 1
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
    raise TestFailedError
  end

  def run
    focused_tests = methods.select { |m| m.start_with?('ftest_') }
    if focused_tests.any?
      run_tests(focused_tests)
    else
      tests = methods.select { |m| m.start_with?('test_') }
      run_tests(tests)
    end
  end

  private

  def run_tests(tests)
    tests.each do |method|
      @test = method
      begin
        send(method)
      rescue => e
        if e.is_a?(TestFailedError)
          @fail += 1
        else
          puts e.message
          puts e.backtrace
          puts "Exception in #{@test}"
          @error += 1
        end
      else
        @success += 1
      end
    end
    puts "#{@success} tests successful (#{@assertions} assertions, #{@fail} failed, #{@error} errored)"
  end
end

class TestCompiler < TestCase
  def test_alias
    assert_eq('foo', AliasTest.new.bar)
  end

  def test_and
    assert_eq(true, true && true)
    assert_eq(false, true && false)
    assert_eq(nil, true && nil)
    assert_eq(false, false && nil)
    assert_eq(nil, nil && false)
    false && fail
  end

  def test_args
    assert_raises(ArgumentError, 'wrong number of arguments (given 0, expected 2)') { arg_required }
    assert_raises(ArgumentError, 'wrong number of arguments (given 1, expected 2)') { arg_required(1) }
  end

  def test_args_destructured
    assert_eq([1, 2, 3, 4], arg_destructure_left([[1, 2, :ignored], 3, :ignored], 4))
    assert_eq([1, 2, 3, 4, 5, 6], arg_destructure_middle(1, [2, [3, 4, :ignored], 5], 6))
    assert_eq([1, 2, 3, 4], arg_destructure_right(1, [2, [3, 4, :ignored], :ignored]))

    assert_eq([:not_an_array, nil, nil, 1], arg_destructure_left(:not_an_array, 1))
    assert_eq([1, :not_an_array, nil, nil, nil, 2], arg_destructure_middle(1, :not_an_array, 2))
    assert_eq([1, :not_an_array, nil, nil], arg_destructure_right(1, :not_an_array))
  end

  def test_args_keyword
    assert_eq([3, 4], arg_keyword_optional(x: 3, y: 4))
    assert_eq([nil, :default], arg_keyword_optional)
    assert_eq([nil, :default], arg_keyword_optional)
    assert_eq([nil, nil], arg_keyword_optional(x: nil, y: nil))
    assert_eq([1, 2], arg_keyword_optional_after_positional(1, y: 2))
    assert_eq([1, :default], arg_keyword_optional_after_positional(1))
    assert_eq([1, :default], arg_keyword_optional_after_positional_optional)
    assert_eq([2, :default], arg_keyword_optional_after_positional_optional(2))
    assert_eq([1, { b: :c }], arg_keyword_splat_after_positional(1, b: :c))
    assert_eq([1, { b: :c }], arg_keyword_splat_after_keyword(a: 1, b: :c))
    assert_eq(1, arg_keyword_splat_unnamed(a: 1, b: :c))
    # TODO: required keyword args
  end

  def test_args_optional
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

  def test_args_splat
    assert_eq([[1, 2], 3], arg_splat_left(1, 2, 3))
    assert_eq([1, [2, 3], 4, 5], arg_splat_middle(1, 2, 3, 4, 5))
    assert_eq([1, [2, 3]], arg_splat_right(1, 2, 3))

    ary = [1, 2, 3]
    assert_eq([1, 2, 3], arg_splat(*ary))
    assert_eq([0, 1, 2, 3], arg_splat(0, *ary))
    assert_eq([1, 2, 3, 4], arg_splat(*ary, 4))
    assert_eq([0, 1, 2, 3, 4], arg_splat(0, *[1, 2, 3], 4))

    assert_eq(1, arg_splat_ignore(1, 2, 3))
  end

  def test_array
    result = ary.map { |i| i * 2 }
    assert_eq([2, 4, 6], result)
  end

  def test_array_splat
    ary = [1, 2, 3]
    i = 1

    *a = *ary
    assert_eq(ary, a)

    a = *ary
    assert_eq(ary, a)

    b = [0, *ary, 4]
    assert_eq([0, 1, 2, 3, 4], b)

    e = [0, *i, 2]
    assert_eq([0, 1, 2], e)

    d = *i
    assert_eq([1], d)
  end

  def test_assignment
    x ||= 1
    x ||= 2
    assert_eq(1, x)

    @y ||= 3
    @y ||= 4
    assert_eq(3, @y)

    $z ||= 5
    $z ||= 6
    assert_eq(5, $z)

    h = {}
    h[:foo] ||= :bar
    h[:foo] ||= :baz
    assert_eq(:bar, h[:foo])
  end

  def test_attr_assignment
    a = AttrAssignTest.new

    a.foo = 1
    assert_eq(1, a.foo)

    AttrAssignTest.bar = 4
    assert_eq(4, AttrAssignTest.bar)
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

    (arg, block) = block_pass2 { 4 }
    assert_eq(nil, arg)
    assert_eq(4, block.call)

    (arg, block) = block_pass2(5) { 6 }
    assert_eq(5, arg)
    assert_eq(6, block.call)
  end

  def test_block_arg_destructure
    [
      [1, 2],
    ].each do |(x, y)|
      assert_eq(1, x)
      assert_eq(2, y)
    end

    ary = [[3, 4]]
    ary.each do |x, y|
      assert_eq(3, x)
      assert_eq(4, y)
    end
    assert_eq([[3, 4]], ary) # don't mutate ary

    [
      5
    ].each do |x, y|
      assert_eq(5, x)
      assert_eq(nil, y)
    end
  end

  def test_block_arg_does_not_overwrite_outer_scope_arg
    z = 0
    [1].each do |z|
      assert_eq(1, z)
    end
    assert_eq(0, z)
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

  def test_break_from_block
    result = block_yield3(1, 2, 3) do
      break 100
    end
    assert_eq(100, result)

    result = block_yield3(1, 2, 3) do
      break
    end
    assert_eq(nil, result)

    result = [3, 2, 1].sort { 1 }.each do
      break 4
    end
    assert_eq(4, result)

    result = [3, 2, 1].sort.each do
      break 5
    end
    assert_eq(5, result)
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
    l3 = lambda do
      break
    end
    assert_eq(nil, l3.call)
  end

  def test_break_from_loop
    result = loop do
      break 200
    end
    assert_eq(200, result)
    result = loop do
      break
    end
    assert_eq(nil, result)
  end

  def test_break_from_proc
    the_proc = proc do
      break 300
    end
    assert_raises(LocalJumpError, 'break from proc-closure') do
      the_proc.call
    end
  end

  def test_break_from_while
    result = while true
      break 200
      break 300
      break 400
    end
    assert_eq(200, result)

    result = while true
      break
    end
    assert_eq(nil, result)
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

    case 1..2
    when Integer
      type = 'int'
    when Range
      type = 'range'
    when String
      type = 'string'
    end
    assert_eq('range', type)
  end

  def test_class
    assert_eq :foo, ClassWithInitialize.new.foo
    assert_eq :bar, ClassWithInitialize.new.bar
    assert_eq 'TestCompiler::ClassWithInitialize', ClassWithInitialize.name
  end

  def test_constant
    assert_eq(:toplevel, CONSTANT)
    assert_eq(:toplevel, ::CONSTANT)
    assert_eq(:toplevel2, ::CONSTANT2)
    assert_eq(:namespaced, Constants::CONSTANT)
    assert_eq(:nested, Constants::Nested::CONSTANT)
  end

  def test_dir
    assert_eq(0, __dir__ =~ %r{^/.*/test/natalie$})
  end

  def test_ensure
    # exception rescued
    result = []
    begin
      result << 1
      non_existent_method
    rescue
      result << 2
    ensure
      result << 3
    end
    assert_eq([1, 2, 3], result)

    # exception not rescued
    result = []
    begin
      begin
        result << 1
        non_existent_method
      ensure
        result << 3
      end
    rescue
    end
    assert_eq([1, 3], result)

    # no exception
    result = []
    begin
      result << 1
    rescue
      result << 2
    ensure
      result << 3
    end
    assert_eq([1, 3], result)

    # ensure caused exception
    result = []
    exception = nil
    begin
      begin
        result << 1
      rescue
        result << 2
      ensure
        result << 3
        raise 'foo'
      end
    rescue => e
      exception = e
    end
    assert_eq([1, 3], result)
    assert_eq('foo', exception.message)
  end

  def test_float
    f = 1.56
    assert_eq(1.56, f)
  end

  def test_global_variable
    $global = 1
    assert_eq(1, $global)
    assert_eq(nil, $non_existent_global)
  end

  def test_hash
    h = { 1 => 2, 3 => 4 }
    h[5] = 6
    assert_eq(3, h.size)
    assert_eq(2, h[1])
    assert_eq(4, h[3])
    assert_eq(6, h[5])
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

  def test_instance_variable
    @ivar = 2
    assert_eq(2, @ivar)
    assert_eq(nil, @non_existent_ivar)
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

  def test_method_in_method
    x = 1
    def should_not_see_outer_scope
      assert_raises(NameError) { x }
      x = 2
    end
    assert_eq(2, should_not_see_outer_scope)
    assert_eq(1, x)
  end

  def test_module
    assert_eq :bar, ModuleToTest.bar
    assert_eq :foo, ClassIncludingModule.new.foo
    assert_eq :foo2, ClassIncludingModule.new.foo2
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

    a = AttrAssignTest.new
    a.foo, b = 2, 3
    assert_eq(2, a.foo)
    assert_eq(3, b)

    *a.foo, b = 2, 3, 4
    assert_eq([2, 3], a.foo)
    assert_eq(4, b)
  end

  def test_next
    result = []
    i = 0
    loop do
      i += 1
      next if i < 5
      result << i
      break if result.size >= 3
    end
    assert_eq([5, 6, 7], result)
  end

  def test_not
    assert_eq(false, (not true))
    assert_eq(false, !true)
    assert_eq(true, 'foo' !~ /bar/)
    assert_eq(true, /bar/ !~ 'foo')
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

  def test_rescue_else
    x = begin
          1
        rescue
          2
        else
          :noop # ensure the whole body is run
          3
        end
    assert_eq(3, x)

    y = begin
          :noop # ensure the whole body is run
          non_existent_method
        rescue
          :noop # ensure the whole body is run
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

  def test_rescue_get_exception
    begin
      raise ArgumentError, 'stuff'
    rescue NoMethodError => e
      fail # should not be here
    rescue ArgumentError => e
      assert_eq(ArgumentError, e.class)
      assert_eq('stuff', e.message)
    end
  end

  def test_regex
    assert_eq(0, 'foo' =~ /foo/)
    assert_eq(0, /F../i =~ 'foo')
    s = 'FOO'
    assert_eq(0, /#{s}/i =~ 'foo')
  end

  def test_return
    x = 0
    def should_return_1
      return 1
      x = 2
    end
    assert_eq(1, should_return_1)
    assert_eq(0, x)

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

    def should_return_in_if(ret)
      return ret if ret
      :nope
    end
    assert_eq(true, should_return_in_if(true))
    assert_eq(:nope, should_return_in_if(false))
  end

  def test_safe_call
    assert_eq('foo', ClassWithClassMethod&.foo)
    assert_eq(nil, nil&.foo)
  end

  def test_sclass
    assert_eq('foo', ClassWithClassMethod.foo)
    assert_eq('bar', ClassWithClassMethod.bar)
  end

  def test_send
    assert_eq('pub', ClassWithPrivateMethod.new.pub)
    assert_raises(NoMethodError) { ClassWithPrivateMethod.new.priv }
  end

  def test_shell_out
    assert_eq('hi', `echo hi`.strip)
    assert_eq('2', `echo #{1 + 1}`.strip)
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

  def test_super
    c = ClassWithSuper.new
    assert_eq('tim', c.super_with_implicit_args('tim'))
    assert_eq(nil, c.super_with_zero_args('tim'))
    assert_eq('tim', c.super_with_given_args('tim'))
    assert_eq('tim', c.super_with_block { 'tim' })
    assert_eq('tim', c.super_with_block_pass { 'tim' })
  end

  def test_svalue
    a = [1, 2, 3]
    arr = *a
    assert_eq([1, 2, 3], arr)
    arr = *[1, 2, 3]
    assert_eq([1, 2, 3], arr)
  end

  def test_until
    y = 0
    until y >= 3
      y += 1
    end
    assert_eq(3, y)

    y = 0
    y += 1 until y >= 3
    assert_eq(3, y)

    y = 0
    begin
      y += 1
    end until true
    assert_eq(1, y)
  end

  def test_variable
    x = 1
    if true
      x += 2
      [3, 4].each { |i| x += i }
      while x % 2 == 0
        x += 1
      end
    end
    assert_eq(11, x)
  end

  def test_while
    x = 0
    while x < 3
      x += 1
    end
    assert_eq(3, x)

    x = 0
    x += 1 while x < 3
    assert_eq(3, x)

    x = 0
    begin
      x += 1
    end while false
    assert_eq(1, x)
  end

  def test_yield
    result = block_yield0 { :hi }
    assert_eq(:hi, result)
    result = block_yield2(1, 2) { |x, y| [x, y] }
    assert_eq([1, 2], result)
    result = block_yield_in_block { |i| i * 2 }
    assert_eq([2], result)
  end

  private

  def ary
    [1, 2, 3]
  end

  def arg_required(a, b)
    [a, b]
  end

  def arg_splat_ignore(a, *)
    a
  end

  def arg_splat_left(*a, b)
    [a, b]
  end

  def arg_splat_middle(a, *b, c, d)
    [a, b, c, d]
  end

  def arg_splat(*args)
    args
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

  def arg_keyword_optional_after_positional_optional(x = 1, y: :default)
    [x, y]
  end

  def arg_keyword_splat_after_positional(a, **rest)
    [a, rest]
  end

  def arg_keyword_splat_after_keyword(a:, **rest)
    [a, rest]
  end

  def arg_keyword_splat_unnamed(a:, **)
    a
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

  def block_yield_in_block
    [1].map do |i|
      yield i
    end
  end

  def block_call(&block)
    block.call
  end

  def block_pass(&block)
    block_call(&block)
  end

  def block_pass2(arg = nil, &block)
    [arg, block]
  end

  def method_raises
    raise 'foo'
  end

  class ClassWithInitialize
    def initialize
      @foo = :foo
    end

    attr_reader :foo

    def super_with_implicit_args(name = nil)
      name
    end

    def super_with_zero_args(name = nil)
      name
    end

    def super_with_given_args(name = nil)
      name
    end

    def super_with_block
      yield
    end

    def super_with_block_pass
      yield
    end
  end

  # reopen class
  class ClassWithInitialize
    def bar; :bar; end
  end

  class ClassWithPrivateMethod
    def pub; 'pub'; end
    private
    def priv; 'priv'; end
  end

  class ClassWithClassMethod
    def self.foo; 'foo'; end
    class << self
      def bar; 'bar'; end
    end
  end

  class ClassWithSuper < ClassWithInitialize
    def initialize
      super
      @last = 'morgan'
    end

    def super_with_implicit_args(name)
      super
    end

    def super_with_zero_args(name)
      super()
    end

    def super_with_given_args(name)
      super(name)
    end

    def super_with_block
      super
    end

    def super_with_block_pass(&block)
      super(&block)
    end
  end

  ::CONSTANT = :toplevel

  class Constants
    CONSTANT = :namespaced
  end

  class Constants::Nested
    CONSTANT = :nested
  end

  class AttrAssignTest
    attr_accessor :foo
    class << self
      def bar; @bar; end
    end
    def self.bar=(bar); @bar = bar; end
  end

  class AliasTest
    def foo
      'foo'
    end
    alias bar foo
  end

  module ModuleToTest
    def foo
      :foo
    end

    class << self
      def bar
        :bar
      end
    end
  end

  # reopen module
  module ModuleToTest
    def foo2
      :foo2
    end
  end

  class ClassIncludingModule
    include ModuleToTest
  end
end

CONSTANT2 = :toplevel2

TestCompiler.new.run
