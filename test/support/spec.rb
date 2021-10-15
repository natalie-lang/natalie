require_relative 'formatters/default_formatter'
require_relative 'formatters/yaml_formatter'
require 'tempfile'

class SpecFailedException < StandardError; end
class UnknownFormatterException < StandardError; end

TOLERANCE = 0.00003
FORMATTERS = ['default', 'yaml']

if ARGV.include?('-f')
  @formatter = ARGV[ARGV.index('-f') + 1]
end

@formatter ||= "default"

unless FORMATTERS.include?(@formatter)
  raise UnknownFormatterException, "#{@formatter} is not supported! Use #{FORMATTERS.join(', ')}"
end

@context = []
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

def describe(description, shared: false, &block)
  if shared
    @shared[description] = block
  else
    parent = @context.last
    @context << Context.new(description, skip: parent && parent.skip)
    yield
    @context.pop
  end
end

alias context describe

def xdescribe(description, &block)
  @context << Context.new(description, skip: true)
  yield
  @context.pop
end

alias xcontext xdescribe

def it(test, &block)
  return xit(test, &block) if @context.last.skip || block.nil?
  @specs << [@context.dup, test, block]
end

def fit(test, &block)
  @specs << [@context.dup, test, block, :focus]
end

def xit(test, &block)
  @specs << [@context.dup, test, nil]
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
  shared_groups.each do |behavior|
    it_behaves_like behavior, @method, @object
  end
end

def specify(&block)
  return xit(nil, &block) if @context.last.skip
  @specs << [@context.dup, nil, block]
end

def nan_value
  0/0.0
end

def infinity_value
  1/0.0
end

def bignum_value(plus = 0)
  0x8000_0000_0000_0000 + plus
end

def ruby_version_is(version)
  without_patch_number = RUBY_VERSION.sub(/\.\d+$/, '')
  if version === without_patch_number
    yield
  end
end

def slow_test
  if ENV['ENABLE_SLOW_TESTS']
    yield
  end
end

def platform_is_not(_)
  yield
end

def not_supported_on(_)
  yield
end

def suppress_warning
  yield
end

def before(type = :each, &block)
  if type == :each
    @context.last.add_before_each(block)
  elsif type == :all
    @context.last.add_before_all(block)
  else
    raise "I don't know how to do before(#{type.inspect})"
  end
end

def after(type = :each, &block)
  if type == :each
    @context.last.add_after_each(block)
  else
    raise "I don't know how to do after(#{type.inspect})"
  end
end

class Matcher
  def initialize(subject, inverted, args)
    @subject = subject
    @inverted = inverted
    @args = args
    if @args.any?
      match_expectation(@args.first)
    end
  end

  def ==(other)
    if @inverted
      neq(other)
    else
      eq(other)
    end
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
    if @inverted
      eq(other)
    else
      neq(other)
    end
  end

  def neq(other)
    if @subject == other
      raise SpecFailedException, @subject.inspect + ' should not (!) be == to ' + other.inspect
    end
  end

  def =~(pattern)
    if @inverted
      not_regex_match(pattern)
    else
      regex_match(pattern)
    end
  end

  def regex_match(pattern)
    if @subject !~ pattern
      raise SpecFailedException, @subject.inspect + ' should match ' + pattern.inspect
    end
  end

  def !~(pattern)
    if @inverted
      regex_match(pattern)
    else
      not_regex_match(pattern)
    end
  end

  def not_regex_match(pattern)
    if @subject =~ pattern
      raise SpecFailedException, @subject.inspect + ' should not (!) match ' + pattern.inspect
    end
  end

  def match_expectation(expectation)
    if @inverted
      expectation.inverted_match(@subject)
    else
      expectation.match(@subject)
    end
  end

  # FIXME: remove these once method_missing is implemented
  def >(other); method_missing(:>, other); end
  def >=(other); method_missing(:>=, other); end
  def <(other); method_missing(:<, other); end
  def <=(other); method_missing(:<=, other); end
  def all?; method_missing(:all?); end
  def any?; method_missing(:any?); end
  def compare_by_identity?; method_missing(:compare_by_identity?); end
  def empty?; method_missing(:empty?); end
  def finite?; method_missing(:finite?); end
  def include?(other); method_missing(:include?, other); end
  def lambda?; method_missing(:lambda?); end
  def nan?; method_missing(:nan?); end
  def none?; method_missing(:none?); end
  def one?; method_missing(:one?); end
  def zero?; method_missing(:zero?); end

  def method_missing(method, *args)
    if args.any?
      than = " than #{args.first.inspect}"
    end
    if !@inverted
      if !@subject.send(method, *args)
        raise SpecFailedException, "#{@subject.inspect} should be #{method}#{than}"
      end
    else
      if @subject.send(method, *args)
        raise SpecFailedException, "#{@subject.inspect} should not be #{method}#{than}"
      end
    end
  end
end

class BeNilExpectation
  def match(subject)
    if subject != nil
      raise SpecFailedException, subject.inspect + ' should be nil'
    end
  end

  def inverted_match(subject)
    if subject == nil
      raise SpecFailedException, subject.inspect + ' should not be nil'
    end
  end
end

class BeKindOfExpectation
  def initialize(klass)
    @klass = klass
  end

  def match(subject)
    if !(@klass === subject)
      raise SpecFailedException, subject.inspect + ' should be a kind of ' + @klass.inspect
    end
  end

  def inverted_match(subject)
    if @klass === subject
      raise SpecFailedException, subject.inspect + ' should not be a kind of ' + @klass.inspect
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
    if (subject.length == 0)
      raise SpecFailedException, subject.inspect + ' should not be empty'
    end
  end
end

class EqualExpectation
  def initialize(other)
    @other = other
  end

  def match(subject)
    if !subject.equal?(@other)
      raise SpecFailedException, subject.inspect + ' should be equal to ' + @other.inspect
    end
  end

  def inverted_match(subject)
    if subject.equal?(@other)
      raise SpecFailedException, subject.inspect + ' should not be equal to ' + @other.inspect
    end
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
    subject.each do |(target, expected)|
      actual = target.send(@method, *@args)
      if actual != expected
        expected_bits = expected.bytes.map { |b| lzpad(b.to_s(2), 8) }.join(" ")
        actual_bits = actual.bytes.map { |b| lzpad(b.to_s(2), 8) }.join(" ")
        raise SpecFailedException, "#{target.inspect} should compute to #{expected_bits}, but it was #{actual_bits}"
      end
    end
  end

  def inverted_match(subject)
    subject.each do |target, expected|
      actual = target.send(@method, *@args)
      if actual == expected
        expected_bits = expected.bytes.map { |b| lzpad(b.to_s(2), 8) }.join(" ")
        raise SpecFailedException, "#{target.inspect} should not compute to #{expected_bits}"
      end
    end
  end

  private

  # TODO: Add % formatting to Natalie :-)
  def lzpad(str, length)
    until str.length == length
      str = '0' + str
    end
    str
  end
end

class BeNanExpectation
  def match(subject)
    if !subject.nan?
      raise SpecFailedException, "#{subject.inspect} should be NaN"
    end
  end

  def inverted_match(subject)
    if subject.nan?
      raise SpecFailedException, "#{subject.inspect} should not be NaN"
    end
  end
end

class RaiseErrorExpectation
  def initialize(klass, message = nil)
    @klass = klass
    @message = message
  end

  def match(subject)
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
        raise SpecFailedException, "#{subject.inspect} should have raised #{@klass.inspect} with message #{@message.inspect}, but the message was #{e.message.inspect}"
      end
    rescue => e
      # FIXME: I don't think this `raise e if` line is needed in MRI -- smells like a Natalie bug
      raise e if e.is_a?(SpecFailedException)
      raise SpecFailedException, "#{subject.inspect} should have raised #{@klass.inspect}, but instead raised #{e.inspect}"
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

# TODO: implement StringIO
class FakeStringIO
  def initialize
    @out = []
  end

  def <<(str)
    @out << str.to_s
  end

  def puts(str)
    self.<<(str + "\n")
  end

  def to_s
    @out.join
  end
end

class ComplainExpectation
  def initialize(message)
    @message = message
  end

  def match(subject)
    out = run(subject)
    case @message
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
    when Regexp
      if out =~ @message
        raise SpecFailedException,
          "#{subject.inspect} should not have printed a warning #{out.inspect}"
      end
    else
      # TODO: might need to implement String matching
      puts "Expected a Regexp to complain but got #{@message.inspect}"
    end
  end

  private

  def run(subject)
    old_stderr = $stderr
    $stderr = FakeStringIO.new
    subject.call
    out = $stderr.to_s
    $stderr = old_stderr
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

        unless exception_to_raise.empty?
          raise exception_to_raise.first
        end

        if block_given? && !yields.empty?
          yields.each do |yield_value|
            yield yield_value
          end
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
      @count_restriction = 1..9999999
    when :twice
      @count_restriction = 2..9999999
    else
      @count_restriction = n..9999999
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
    unless @pass
      raise SpecFailedException, "#{@subject.inspect} should have received #{@message}"
    end
  end
end

# what method does that need?
class IncludeExpectation
  def initialize(values)
    @values = values
  end

  def match(subject)
    @values.each do |value|
      unless subject.include?(value)
        raise SpecFailedException, "#{subject.inspect} should include #{@value.inspect}"
      end
    end
  end

  def inverted_match(subject)
    @values.each do |value|
      if subject.include?(value)
        raise SpecFailedException, "#{subject.inspect} should not include #{@value.inspect}"
      end
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

  def eql(other)
    EqlExpectation.new(other)
  end

  def be_empty()
    BeEmptyExpectation.new
  end

  def equal(other)
    EqualExpectation.new(other)
  end

  def raise_error(klass = nil, message = nil)
    RaiseErrorExpectation.new(klass, message)
  end

  def complain(message)
    ComplainExpectation.new(message)
  end

  def should_receive(message)
    Stub.new(self, message).tap do |stub|
      $expectations << stub
    end
  end

  def should_not_receive(message)
    define_singleton_method(message) do
      raise SpecFailedException, "#{message} should not have been sent to #{inspect}"
    end
  end

  def include(*values)
    IncludeExpectation.new(values)
  end

  # FIXME: the above method is visible to tests in **Natalie** but not MRI, and I don't know why.
  # This alias is here so that MRI can see it. We should figure out why Natalie can see 'include'
  # but MRI cannot. (That's a bug.)
  alias include_all include

  def stub!(message)
    Stub.new(self, message)
  end
end

class MockObject
end

def mock(name)
  MockObject.new.tap do |obj|
    obj.define_singleton_method(:inspect) do
      "<mock: #{name}>"
    end
  end
end

def mock_int(value)
  mock("int #{value}").tap do |obj|
    obj.should_receive(:to_int).at_least(:once).and_return(value.to_int)
  end
end

def mock_numeric(name)
  mock(name)
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
    (context, test, fn, focus) = test
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
        context.each do |con|
          con.before_each.each do |b|
            b.call
          end
        end
        $expectations = []
        fn.call

        $expectations.each do |expectation|
          expectation.validate!
        end
        context.each do |con|
          con.after_each.each do |a|
            a.call
          end
        end
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

at_exit do
  run_specs
end
