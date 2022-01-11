require_relative 'formatters/default_formatter'
require_relative 'formatters/yaml_formatter'
require_relative 'mspec'
require_relative 'platform_guard'
require_relative 'version'
require 'tempfile'

class SpecFailedException < StandardError
end
class UnknownFormatterException < StandardError
end

TOLERANCE = 0.00003
FORMATTERS = %w[default yaml]

@formatter = ARGV[ARGV.index('-f') + 1] if ARGV.include?('-f')

@formatter ||= 'default'

unless FORMATTERS.include?(@formatter)
  raise UnknownFormatterException, "#{@formatter} is not supported! Use #{FORMATTERS.join(', ')}"
end

$context = []
@shared = {}
@specs = []

$expectations = []

class Context
  def initialize(description, skip: false)
    @description = description
    @before_each = []
    @before_all = []
    @after_each = []
    @skip = skip
  end

  attr_reader :description, :before_each, :before_all, :after_each, :skip

  def add_before_each(block)
    @before_each << block
  end

  def add_before_all(block)
    @before_all << block
  end

  def add_after_each(block)
    @after_each << block
  end

  def to_s
    @description
  end
end

class ScratchPad
  def self.record(item)
    @record = item
  end

  def self.recorded
    @record
  end

  def self.clear
    @recorded = nil
  end

  def self.<<(item)
    @record << item
  end
end

def ci?
  !!ENV['CI']
end

def describe(description, shared: false, &block)
  if shared
    @shared[description] = block
  else
    parent = $context.last
    $context << Context.new(description, skip: parent && parent.skip)
    yield
    $context.pop
  end
end

alias context describe

def xdescribe(description, &block)
  $context << Context.new(description, skip: true)
  yield
  $context.pop
end

alias xcontext xdescribe

def it(test, &block)
  return xit(test, &block) if $context.last.skip || block.nil?
  @specs << [$context.dup, test, block]
end

def fit(test, &block)
  raise 'Focused spec in CI detected. Please remove it!' if ci?
  @specs << [$context.dup, test, block, :focus]
end

def xit(test, &block)
  @specs << [$context.dup, test, nil]
end

def it_behaves_like(behavior, method, obj = nil)
  before :all do
    @method = method if method
    @object = obj if obj
  end

  block = @shared[behavior]
  if block
    block.call
  else
    raise "Cannot find shared behavior: #{behavior.inspect} (available: #{@shared.keys.inspect})"
  end
end

def it_should_behave_like(*shared_groups)
  shared_groups.each { |behavior| it_behaves_like behavior, @method, @object }
end

def specify(test = nil, &block)
  return xit(test, &block) if $context.last.skip
  @specs << [$context.dup, test, block]
end

def with_feature(name)
  yield if MSpec.features[name]
end

def nan_value
  0 / 0.0
end

def infinity_value
  1 / 0.0
end

def bignum_value(plus = 0)
  0x8000_0000_0000_0000 + plus
end

def fixnum_max
  9_223_372_036_854_775_807
end

def fixnum_min
  -9_223_372_036_854_775_807
end

def max_long
  # 2**(0.size * 8 - 1) - 1
  # NATFIXME: Support Integer#size
  # NATFIXME: Make Integer#** spec compliant with bignums
  (2**(8 * 8 - 2)) * 2 - 1
end

def min_long
  # -(2**(0.size * 8 - 1))
  # NATFIXME: Support Integer#size
  # NATFIXME: Make Integer#** spec compliant with bignums
  -((2**(8 * 8 - 2)) * 2)
end

def ruby_version_is(version)
  ruby_version = SpecVersion.new RUBY_VERSION
  case version
  when String
    requirement = SpecVersion.new version
    yield if ruby_version >= requirement
  when Range
    a = SpecVersion.new version.begin
    b = SpecVersion.new version.end
    yield if ruby_version >= a && (version.exclude_end? ? ruby_version < b : ruby_version <= b)
  else
    raise "version must be a String or Range but was a #{version.class}"
  end
end

def slow_test
  yield if ENV['ENABLE_SLOW_TESTS']
end

def platform_is(platform)
  if platform.is_a?(Hash)
    yield if platform[:wordsize] == 64
  elsif platform == :linux
    yield if RUBY_PLATFORM =~ /linux/
  end
end

def platform_is_not(*)
  yield
end

def not_supported_on(*)
  yield
end

def as_user
  yield
end

def suppress_warning
  old_stderr = $stderr
  $stderr = IOStub.new
  yield
  $stderr = old_stderr
end

def before(type = :each, &block)
  if type == :each
    $context.last.add_before_each(block)
  elsif type == :all
    $context.last.add_before_all(block)
  else
    raise "I don't know how to do before(#{type.inspect})"
  end
end

def after(type = :each, &block)
  if type == :each
    $context.last.add_after_each(block)
  else
    raise "I don't know how to do after(#{type.inspect})"
  end
end

def guard(proc)
  yield if proc.call
end

def quarantine!
  # do nothing
end

def flunk
  raise SpecFailedException
end

class Matcher
  def initialize(subject, inverted, args)
    @subject = subject
    @inverted = inverted
    @args = args
    match_expectation(@args.first) if @args.any?
  end

  def ==(other)
    @inverted ? neq(other) : eq(other)
  end

  def eq(other)
    if @subject != other
      if @subject.is_a?(String) && other.is_a?(String) && (@subject.size > 50 || other.size > 50)
        subject_file = Tempfile.new('subject') { |f| f.write(@subject) }
        other_file = Tempfile.new('other') { |f| f.write(other) }
        puts `diff #{other_file.path} #{subject_file.path}`
        File.unlink(subject_file.path)
        File.unlink(other_file.path)
        raise SpecFailedException, 'two strings should match'
      else
        raise SpecFailedException, @subject.inspect + ' should be == to ' + other.inspect
      end
    end
  end

  def !=(other)
    @inverted ? eq(other) : neq(other)
  end

  def neq(other)
    raise SpecFailedException, @subject.inspect + ' should not (!) be == to ' + other.inspect if @subject == other
  end

  def =~(pattern)
    @inverted ? not_regex_match(pattern) : regex_match(pattern)
  end

  def regex_match(pattern)
    raise SpecFailedException, @subject.inspect + ' should match ' + pattern.inspect if @subject !~ pattern
  end

  def !~(pattern)
    @inverted ? regex_match(pattern) : not_regex_match(pattern)
  end

  def not_regex_match(pattern)
    raise SpecFailedException, @subject.inspect + ' should not (!) match ' + pattern.inspect if @subject =~ pattern
  end

  def match_expectation(expectation)
    @inverted ? expectation.inverted_match(@subject) : expectation.match(@subject)
  end

  # FIXME: remove these once method_missing is implemented
  def >(other)
    method_missing(:>, other)
  end
  def >=(other)
    method_missing(:>=, other)
  end
  def <(other)
    method_missing(:<, other)
  end
  def <=(other)
    method_missing(:<=, other)
  end
  def all?
    method_missing(:all?)
  end
  def any?
    method_missing(:any?)
  end
  def compare_by_identity?
    method_missing(:compare_by_identity?)
  end
  def casefold?
    method_missing(:casefold?)
  end
  def empty?
    method_missing(:empty?)
  end
  def exclude_end?
    method_missing(:exclude_end?)
  end
  def finite?
    method_missing(:finite?)
  end
  def include?(other)
    method_missing(:include?, other)
  end
  def initialized?
    method_missing(:initialized?)
  end
  def integer?
    method_missing(:integer?)
  end
  def lambda?
    method_missing(:lambda?)
  end
  def nan?
    method_missing(:nan?)
  end
  def negative?
    method_missing(:negative?)
  end
  def none?
    method_missing(:none?)
  end
  def one?
    method_missing(:one?)
  end
  def positive?
    method_missing(:positive?)
  end
  def real?
    method_missing(:real?)
  end
  def start_with?(other)
    method_missing(:start_with?, other)
  end
  def success?
    method_missing(:success?)
  end
  def zero?
    method_missing(:zero?)
  end

  def method_missing(method, *args)
    than = " than #{args.first.inspect}" if args.any?
    if !@inverted
      raise SpecFailedException, "#{@subject.inspect} should be #{method}#{than}" if !@subject.send(method, *args)
    else
      raise SpecFailedException, "#{@subject.inspect} should not be #{method}#{than}" if @subject.send(method, *args)
    end
  end
end

class BeNilExpectation
  def match(subject)
    raise SpecFailedException, subject.inspect + ' should be nil' if subject != nil
  end

  def inverted_match(subject)
    raise SpecFailedException, subject.inspect + ' should not be nil' if subject == nil
  end
end

class BeKindOfExpectation
  def initialize(klass)
    @klass = klass
  end

  def match(subject)
    raise SpecFailedException, subject.inspect + ' should be a kind of ' + @klass.inspect if !(@klass === subject)
  end

  def inverted_match(subject)
    raise SpecFailedException, subject.inspect + ' should not be a kind of ' + @klass.inspect if @klass === subject
  end
end

class BeAncestorOfExpectation
  def initialize(klass)
    @klass = klass
  end

  def match(subject)
    unless @klass.ancestors.include?(subject)
      raise SpecFailedException, subject.inspect + ' should be an ancestor of ' + @klass.inspect
    end
  end

  def inverted_match(subject)
    if @klass.ancestors.include?(subject)
      raise SpecFailedException, subject.inspect + ' should not to be an ancestor of ' + @klass.inspect
    end
  end
end

class EqlExpectation
  def initialize(other)
    @other = other
  end

  def match(subject)
    if !subject.eql?(@other)
      raise SpecFailedException, subject.inspect + ' should be eql (same type) to ' + @other.inspect
    end
  end

  def inverted_match(subject)
    if subject.eql?(@other)
      raise SpecFailedException, subject.inspect + ' should not be eql (same type) to ' + @other.inspect
    end
  end
end

class BeEmptyExpectation
  def match(subject)
    if (subject.length > 0)
      raise SpecFailedException, subject.inspect + ' should be empty but has size ' + subject.length
    end
  end

  def inverted_match(subject)
    raise SpecFailedException, subject.inspect + ' should not be empty' if (subject.length == 0)
  end
end

class EqualExpectation
  def initialize(other)
    @other = other
  end

  def match(subject)
    raise SpecFailedException, subject.inspect + ' should be equal to ' + @other.inspect if !subject.equal?(@other)
  end

  def inverted_match(subject)
    raise SpecFailedException, subject.inspect + ' should not be equal to ' + @other.inspect if subject.equal?(@other)
  end
end

class BeCloseExpectation
  def initialize(target, tolerance = TOLERANCE)
    @target = target
    @tolerance = tolerance
  end

  def match(subject)
    if subject.abs - @target.abs > @tolerance
      raise SpecFailedException, "#{subject.inspect} should be within #{@tolerance} of #{@target}"
    end
  end

  def inverted_match(subject)
    if subject.abs - @target.abs <= @tolerance
      raise SpecFailedException, "#{subject.inspect} should not be within #{@tolerance} of #{@target}"
    end
  end
end

class BeComputedByExpectation
  def initialize(method, args)
    @method = method
    @args = args
  end

  def match(subject)
    subject.each do |(target, *args, expected)|
      actual = target.send(@method, *(@args + args))
      if actual != expected
        expected_bits =
          if expected.methods.include?(:bytes)
            expected.bytes.map { |b| lzpad(b.to_s(2), 8) }.join(' ')
          else
            expected.inspect
          end

        actual_bits =
          actual.methods.include?(:bytes) ? actual.bytes.map { |b| lzpad(b.to_s(2), 8) }.join(' ') : actual.inspect

        raise SpecFailedException, "#{target.inspect} should compute to #{expected_bits}, but it was #{actual_bits}"
      end
    end
  end

  def inverted_match(subject)
    subject.each do |target, expected|
      actual = target.send(@method, *@args)
      if actual == expected
        expected_bits = expected.bytes.map { |b| lzpad(b.to_s(2), 8) }.join(' ')
        raise SpecFailedException, "#{target.inspect} should not compute to #{expected_bits}"
      end
    end
  end

  private

  # TODO: Add % formatting to Natalie :-)
  def lzpad(str, length)
    str = '0' + str until str.length == length
    str
  end
end

class BeNanExpectation
  def match(subject)
    raise SpecFailedException, "#{subject.inspect} should be NaN" if !subject.nan?
  end

  def inverted_match(subject)
    raise SpecFailedException, "#{subject.inspect} should not be NaN" if subject.nan?
  end
end

class RaiseErrorExpectation
  def initialize(klass, message = nil, &block)
    @klass = klass
    @message = message
    @block = block
  end

  def match(subject)
    if @block
      # given a block, we rescue everything!
      begin
        subject.call
      rescue Exception => e
        @block.call(e)
      end
      return
    end

    begin
      subject.call
    rescue @klass => e
      if @message.nil?
        nil # good
      elsif @message == e.message
        nil # good
      elsif @message.is_a?(Regexp) && @message =~ e.message
        nil # good
      else
        raise SpecFailedException,
              "#{subject.inspect} should have raised #{@klass.inspect} with message #{@message.inspect}, but the message was #{e.message.inspect}"
      end
    rescue => e
      # FIXME: I don't think this `raise e if` line is needed in MRI -- smells like a Natalie bug
      raise e if e.is_a?(SpecFailedException)
      raise SpecFailedException,
            "#{subject.inspect} should have raised #{@klass.inspect}, but instead raised #{e.inspect}"
    else
      raise SpecFailedException, "#{subject.inspect} should have raised #{@klass.inspect}, but instead raised nothing"
    end
  end

  def inverted_match(subject)
    if @klass
      begin
        subject.call
      rescue @klass
        raise SpecFailedException, subject.inspect + ' should not have raised ' + @klass.inspect
      end
    else
      begin
        subject.call
      rescue => e
        # FIXME: same bug as above
        raise e if e.is_a?(SpecFailedException)
        raise SpecFailedException, "#{subject.inspect} should not have raised any errors"
      end
    end
  end
end

class IOStub
  def initialize
    @out = []
  end

  def <<(str)
    @out << str.to_s
  end
  alias write <<

  def puts(str)
    self.<<(str + "\n")
  end

  def to_s
    @out.join
  end

  def =~(r)
    r =~ to_s
  end
end

class ComplainExpectation
  def initialize(message = nil, verbose: false)
    @message = message
    @verbose = verbose
  end

  def match(subject)
    out = run(subject)
    case @message
    when nil
      raise SpecFailedException, 'Expected a warning but received none' if out.empty?
    when Regexp
      unless out =~ @message
        raise SpecFailedException,
              "#{subject.inspect} should have printed a warning #{@message.inspect}, but the output was #{out.inspect}"
      end
    else
      # TODO: might need to implement String matching
      puts "Expected a Regexp to complain but got #{@message.inspect}"
    end
  end

  def inverted_match(subject)
    out = run(subject)
    case @message
    when nil
      raise SpecFailedException, "Unexpected warning: #{out}" unless out.empty?
    when Regexp
      if out =~ @message
        raise SpecFailedException, "#{subject.inspect} should not have printed a warning #{out.inspect}"
      end
    else
      # TODO: might need to implement String matching
      puts "Expected a Regexp to complain but got #{@message.inspect}"
    end
  end

  private

  def run(subject)
    old_stderr = $stderr
    old_verbose = $VERBOSE
    $stderr = IOStub.new
    $VERBOSE = @verbose
    subject.call
    out = $stderr.to_s
    $stderr = old_stderr
    $VERBOSE = old_verbose
    out
  end
end

class Stub
  def initialize(subject, message)
    @subject = subject
    @message = message
    @pass = false
    @args = nil
    @count = 0
    at_least(1)
    @yield_values = []
    @results = []
    @raises = []

    increment = -> { @count += 1 }
    expected_args = @args
    should_receive_called = -> { @pass = @count_restriction.nil? || @count_restriction === @count }
    return_values = @results
    result = -> { return_values.size > 1 ? return_values[@count - 1] : return_values[0] }
    yields = @yield_values
    exception_to_raise = @raises
    @subject.define_singleton_method(@message) do |*args|
      if expected_args.nil? || args == expected_args
        increment.()
        should_receive_called.()

        raise exception_to_raise.first unless exception_to_raise.empty?

        if block_given? && !yields.empty?
          yields.each { |yield_value| yield yield_value }
        elsif !return_values.empty?
          result.()
        end
      else
        fail
      end
    end
  end

  def with(*args)
    @args = args
    self
  end

  def and_return(*results)
    @results.concat(results)
    self
  end

  def and_raise(exception)
    @raises << exception
    self
  end

  def and_yield(value)
    @yield_values << value
    self
  end

  def at_most(n)
    case n
    when :once
      @count_restriction = 0..1
    when :twice
      @count_restriction = 0..2
    else
      @count_restriction = 0..n
    end
    self
  end

  def at_least(n)
    # FIXME: support endless ranges
    case n
    when :once
      @count_restriction = 1..9_999_999
    when :twice
      @count_restriction = 2..9_999_999
    else
      @count_restriction = n..9_999_999
    end
    self
  end

  def times
    self
  end

  def once
    exactly(1)
    self
  end

  def twice
    exactly(2)
    self
  end

  def exactly(n)
    @count_restriction = n
    self
  end

  def any_number_of_times
    @pass = true
    @count_restriction = nil
    self
  end

  def validate!
    raise SpecFailedException, "#{@subject.inspect} should have received #{@message}" unless @pass
  end
end

# what method does that need?
class IncludeExpectation
  def initialize(values)
    @values = values
  end

  def match(subject)
    @values.each do |value|
      raise SpecFailedException, "#{subject.inspect} should include #{value.inspect}" unless subject.include?(value)
    end
  end

  def inverted_match(subject)
    @values.each do |value|
      raise SpecFailedException, "#{subject.inspect} should not include #{value.inspect}" if subject.include?(value)
    end
  end
end

class HaveMethodExpectation
  def initialize(method)
    @method = method.to_sym
  end

  def match(subject)
    unless subject.methods.include?(@method)
      raise SpecFailedException, "Expected #{subject} to have method '#{@method.to_s}' but it does not"
    end
  end

  def inverted_match(subject)
    if subject.methods.include?(@method)
      raise SpecFailedException, "Expected #{subject} NOT to have method '#{@method.to_s}' but it does"
    end
  end
end

class HaveInstanceMethodExpectation
  def initialize(method, include_super)
    @method = method.to_sym
    @include_super = include_super
  end

  def match(subject)
    unless subject.instance_methods(@include_super).include?(@method)
      raise SpecFailedException, "Expected #{subject} to have instance method '#{@method.to_s}' but it does not"
    end
  end

  def inverted_match(subject)
    if subject.instance_methods(@include_super).include?(@method)
      raise SpecFailedException, "Expected #{subject} NOT to have instance method '#{@method.to_s}' but it does"
    end
  end
end

class HavePrivateInstanceMethodExpectation
  def initialize(method, include_super)
    @method = method.to_sym
    @include_super = include_super
  end

  def match(subject)
    unless subject.private_instance_methods(@include_super).include?(@method)
      raise SpecFailedException, "Expected #{subject} to have private instance method '#{@method.to_s}' but it does not"
    end
  end

  def inverted_match(subject)
    if subject.private_instance_methods(@include_super).include?(@method)
      raise SpecFailedException, "Expected #{subject} NOT to have private instance method '#{@method.to_s}' but it does"
    end
  end
end

class HaveProtectedInstanceMethodExpectation
  def initialize(method, include_super)
    @method = method.to_sym
    @include_super = include_super
  end

  def match(subject)
    unless subject.protected_instance_methods(@include_super).include?(@method)
      raise SpecFailedException,
            "Expected #{subject} to have protected instance method '#{@method.to_s}' but it does not"
    end
  end

  def inverted_match(subject)
    if subject.protected_instance_methods(@include_super).include?(@method)
      raise SpecFailedException,
            "Expected #{subject} NOT to have protected instance method '#{@method.to_s}' but it does"
    end
  end
end

class HavePublicInstanceMethodExpectation
  def initialize(method, include_super)
    @method = method.to_sym
    @include_super = include_super
  end

  def match(subject)
    unless subject.public_instance_methods(@include_super).include?(@method)
      raise SpecFailedException, "Expected #{subject} to have public instance method '#{@method.to_s}' but it does not"
    end
  end

  def inverted_match(subject)
    if subject.public_instance_methods(@include_super).include?(@method)
      raise SpecFailedException, "Expected #{subject} NOT to have public instance method '#{@method.to_s}' but it does"
    end
  end
end

class Object
  def should(*args)
    Matcher.new(self, false, args)
  end

  def should_not(*args)
    Matcher.new(self, true, args)
  end

  def be_nil
    BeNilExpectation.new
  end

  def be_nan
    BeNanExpectation.new
  end

  def be_negative_zero
    EqlExpectation.new(-0.0)
  end

  def be_positive_zero
    EqlExpectation.new(0.0)
  end

  def be_true
    EqualExpectation.new(true)
  end

  def be_false
    EqualExpectation.new(false)
  end

  def be_close(target, tolerance)
    BeCloseExpectation.new(target, tolerance)
  end

  def be_computed_by(method, *args)
    BeComputedByExpectation.new(method, args)
  end

  def be_kind_of(klass)
    BeKindOfExpectation.new(klass)
  end

  def be_an_instance_of(klass)
    BeKindOfExpectation.new(klass)
  end

  def be_ancestor_of(klass)
    BeAncestorOfExpectation.new(klass)
  end

  def eql(other)
    EqlExpectation.new(other)
  end

  def be_empty()
    BeEmptyExpectation.new
  end

  def equal(other)
    EqualExpectation.new(other)
  end

  def raise_error(klass = nil, message = nil, &block)
    RaiseErrorExpectation.new(klass, message, &block)
  end

  def complain(message = nil, verbose: false)
    ComplainExpectation.new(message, verbose: verbose)
  end

  def should_receive(message)
    Stub.new(self, message).tap { |stub| $expectations << stub }
  end

  def should_not_receive(message)
    define_singleton_method(message) { raise SpecFailedException, "#{message} should not have been sent to #{inspect}" }
  end

  def include(*values)
    IncludeExpectation.new(values)
  end

  # FIXME: the above method is visible to tests in **Natalie** but not MRI, and I don't know why.
  # This alias is here so that MRI can see it. We should figure out why Natalie can see 'include'
  # but MRI cannot. (That's a bug.)
  alias include_all include

  def have_method(method)
    HaveMethodExpectation.new(method)
  end

  def have_instance_method(method, include_super = true)
    HaveInstanceMethodExpectation.new(method, include_super)
  end

  def have_private_instance_method(method, include_super = true)
    HavePrivateInstanceMethodExpectation.new(method, include_super)
  end

  def have_protected_instance_method(method, include_super = true)
    HaveProtectedInstanceMethodExpectation.new(method, include_super)
  end

  def have_public_instance_method(method, include_super = true)
    HavePublicInstanceMethodExpectation.new(method, include_super)
  end

  def stub!(message)
    Stub.new(self, message)
  end
end

module Mock
  def initialize(name)
    @name = name
  end

  def inspect
    "<mock: #{@name}>"
  end
end

class MockObject
  include Mock
end

class MockNumeric < Numeric
  include Mock
end

def mock(name)
  MockObject.new(name)
end

def mock_int(value)
  mock("int #{value}").tap { |obj| obj.should_receive(:to_int).at_least(:once).and_return(value.to_int) }
end

def mock_numeric(name)
  MockNumeric.new(name)
end

def run_specs
  @failures = []
  @errors = []
  @skipped = []
  @test_count = 0

  before_all_done = []
  any_focused = @specs.any? { |_, _, _, focus| focus }

  formatter =
    case @formatter
    when 'yaml'
      YamlFormatter.new
    else
      DefaultFormatter.new
    end

  @specs.each do |test|
    context, test, fn, focus = test
    next if any_focused && !focus
    if fn
      @test_count += 1
      begin
        context.each do |con|
          con.before_all.each do |b|
            unless before_all_done.include?(b)
              b.call
              before_all_done << b
            end
          end
        end
        context.each { |con| con.before_each.each { |b| b.call } }
        $expectations = []
        fn.call

        $expectations.each { |expectation| expectation.validate! }
        context.each { |con| con.after_each.each { |a| a.call } }
      rescue SpecFailedException => e
        @failures << [context, test, e]
        formatter.print_failure(*@failures.last)
      rescue Exception => e
        @errors << [context, test, e]
        formatter.print_error(*@errors.last)
      else
        formatter.print_success(context, test)
      end
    else
      @skipped << [context, test]
      formatter.print_skipped(*@skipped.last)
    end
  end

  formatter.print_finish(@test_count, @failures, @errors, @skipped)
end

at_exit { run_specs }
