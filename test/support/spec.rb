require_relative 'formatters/default_formatter'
require_relative 'formatters/yaml_formatter'
require_relative 'mspec'
require_relative 'platform_guard'
require_relative 'version'
require_relative 'spec_helpers/fs'
require_relative 'spec_helpers/mock_to_path'
require_relative 'spec_helpers/tmp'
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
    @after_all = []
    @skip = skip
  end

  attr_reader :description, :before_each, :before_all, :after_each, :after_all, :skip

  def add_before_each(block)
    @before_each << block
  end

  def add_before_all(block)
    @before_all << block
  end

  def add_after_each(block)
    @after_each << block
  end

  def add_after_all(block)
    @after_all << block
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
    @record = nil
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

def with_timezone(zone, offset = nil)
  old_tz = ENV['TZ']

  if offset
    zone += "#{(-offset).to_s}:00:00"
  end

  ENV['TZ'] = zone

  begin
    yield
  ensure
    # Natalie does not allow setting ENV values that are not strings?
    if old_tz.is_a?(String)
      ENV['TZ'] = old_tz
    end
  end
end

def nan_value
  0 / 0.0
end

def infinity_value
  1 / 0.0
end

def bignum_value(plus = 0)
  (2 ** 64) + plus
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
  -((2**(8 * 8 - 2)) * 2)
end

def ruby_exe(code, args: nil, exit_status: 0)
  Tempfile.create('ruby_exe.rb') do |file|
    file.write(code)
    file.rewind

    binary = ENV['NAT_BINARY'] || 'bin/natalie'

    output = `#{binary} #{file.path} #{args}`

    if $?.exitstatus != exit_status
      raise SpecFailedException, "Expected exit status #{exit_status} but actual is #{$?.exitstatus}" 
    end
    output
  end
end

def ruby_version_is(version, &block)
  version_is(RUBY_VERSION, version, &block)
end

def version_is(version, requirement_version)
  version = SpecVersion.new(version)
  case requirement_version
  when String
    requirement = SpecVersion.new requirement_version
    yield if version >= requirement
  when Range
    a = SpecVersion.new requirement_version.begin
    b = SpecVersion.new requirement_version.end
    yield if version >= a && (requirement_version.exclude_end? ? version < b : version <= b)
  else
    raise "version must be a String or Range but was a #{requirement_version.class}"
  end
end

# We do not want replicate ruby bugs
def ruby_bug(*); end

def slow_test
  yield if ENV['ENABLE_SLOW_TESTS']
end

def _platform_match(*args)
   options, platforms = if args.last.is_a?(Hash)
                          [args.last, args[0..-2]]
                        else
                          [{}, args]
                        end
   return true if options[:wordsize] == 64 || options[:pointer_size] == 64
   return true if platforms.include?(:windows) && RUBY_PLATFORM =~ /(mswin|mingw)/
   return true if platforms.include?(:linux) && RUBY_PLATFORM =~ /linux/
   return true if platforms.include?(:darwin) && RUBY_PLATFORM =~ /darwin/i
   return true if platforms.include?(:openbsd) && RUBY_PLATFORM =~ /openbsd/i
   return true if platforms.include?(:freebsd) && RUBY_PLATFORM =~ /freebsd/i
   return true if platforms.include?(:netbsd) && RUBY_PLATFORM =~ /netbsd/i
   return true if platforms.include?(:bsd) && RUBY_PLATFORM =~ /(bsd|darwin)/i
   # TODO: cygwin, android, solaris and aix are currently uncovered
   false
end

def platform_is(*args)
  matched = _platform_match(*args)
  return matched unless block_given?
  if matched
    yield
  end
end

def platform_is_not(*args)
  not_matched = !_platform_match(*args)
  return not_matched unless block_given?
  if not_matched
    yield
  end
end

def not_supported_on(*)
  yield
end

# NATFIXME
def kernel_version_is(*)
  false
end

def as_user
  if Process.euid != 0
    yield
  end
end

def as_superuser
  if Process.euid == 0
    yield
  end
end

def as_real_superuser
  if Process.uid == 0
    yield
  end
end

def little_endian
  yield
end

def big_endian
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
  elsif type == :all
    $context.last.add_after_all(block)
  else
    raise "I don't know how to do after(#{type.inspect})"
  end
end

def guard(proc)
  yield if proc.call
end

def guard_not(proc)
  yield unless proc.call
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
        diff(@subject, other)
        raise SpecFailedException, 'two strings should match'
      elsif @subject.is_a?(Array) && other.is_a?(Array) && (@subject.size > 1 || other.size > 1)
        diff(
          "[\n" + @subject.map(&:inspect).join("\n") + "\n]",
          "[\n" + other.map(&:inspect).join("\n") + "\n]",
        )
        raise SpecFailedException, @subject.inspect + ' should be == to ' + other.inspect
      else
        raise SpecFailedException, @subject.inspect + ' should be == to ' + other.inspect
      end
    end
  end

  def diff(actual, expected)
    actual_file = Tempfile.create('actual')
    actual_file.write(actual)
    actual_file.close
    expected_file = Tempfile.create('expected')
    expected_file.write(expected)
    expected_file.close
    puts `diff #{expected_file.path} #{actual_file.path}`
    File.unlink(actual_file.path)
    File.unlink(expected_file.path)
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

  def method_missing(method, *args)
    target = if args.any?
               case method
               when :<, :<=, :>, :>=
                 "be #{method} than #{args.first.inspect}"
               else
                 "#{method} #{args.first.inspect}"
               end
             else
               "be #{method.to_s}"
             end
    if !@inverted
      raise SpecFailedException, "#{@subject.inspect} should #{target}" if !@subject.send(method, *args)
    else
      raise SpecFailedException, "#{@subject.inspect} should not #{target}" if @subject.send(method, *args)
    end
  end

  undef_method :equal?
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

class TrueFalseExpectation
  def match(subject)
    unless subject == true || subject == false
      raise SpecFailedException, subject.inspect + ' should be true or false'
    end
  end

  def inverted_match(subject)
    if subject == true || subject == false
      raise SpecFailedException, subject.inspect + ' should not be true or false'
    end
  end
end

class BeCloseExpectation
  def initialize(target, tolerance = TOLERANCE)
    @target = target
    @tolerance = tolerance
  end

  def match(subject)
    if (subject - @target).abs > @tolerance
      raise SpecFailedException, "#{subject.inspect} should be within #{@tolerance} of #{@target}"
    end
  end

  def inverted_match(subject)
    if (subject - @target).abs <= @tolerance
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

class BeInfinityExpectation
  def initialize(sign_of_infinity)
    @sign_of_infinity = sign_of_infinity
  end

  def match(subject)
    raise SpecFailedException, "#{subject.inspect} should be #{"-" if negative?}Infinity" unless expected_infinity?(subject)
  end

  private

  def expected_infinity?(subject)
    subject.kind_of?(Float) && subject.infinite? == @sign_of_infinity
  end

  def negative?
    @sign_of_infinity == -1
  end
end


class OutputExpectation
  def initialize(expected)
    @expected = expected
  end

  def match(subject)
    actual = capture_output do
      subject.call
    end
    if actual != @expected
      raise SpecFailedException, "expected $stdout to get #{@expected.inspect} but it got #{actual.inspect} instead"
    end
  end

  def inverted_match(subject)
    actual = capture_output do
      subject.call
    end
    if actual == @expected
      raise SpecFailedException, "expected $stdout not to get #{@expected.inspect} but it did"
    end
  end

  private

  def capture_output
    stub = IOStub.new
    old_stdout = $stdout
    begin
      $stdout = stub
      yield
    ensure
      $stdout = old_stdout
    end
    stub.to_s
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
  alias print <<

  def puts(*args)
    args.each do |arg|
      self.<<(arg.to_s + "\n")
    end
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

class StubRegistry
  attr_reader :stubs

  def initialize
    @stubs = {}
  end

  def register(stub)
    $expectations << stub

    if stub.message == :object_id
      # If some spec has to stub object_id we have to rethink the idea of the StubRegistry class. We currently use the object_id
      # method do build a hash key from the object id and the stub message.
      raise SpecFailedException, "Cannot register stub for :object_id. This method is used internally and thus cannot be stubbed."
    end

    key = [stub.subject.object_id, stub.message]
    if stubs.key?(key)
      # We are prepending stubs here to ensure that new stubs get matched first.
      stubs[key].prepend(stub)
    else
      stubs[key] = [stub]

      get_stubs = ->(subject, method) { @stubs[[subject.object_id, method]] }
      stub.subject.define_singleton_method(stub.message) do |*args, &block|
        matched_stub = get_stubs.(stub.subject, stub.message).find { |stub| stub.match?(*args) }

        if matched_stub
          matched_stub.execute(&block)
        else
          super(*args, &block)
        end
      end
    end
  end

  def reset
    @stubs.clear
  end
end

class Stub
  attr_reader :subject, :message

  def initialize(subject, message)
    @subject = subject
    @message = message
    @args = nil
    @count = 0
    at_least(1)
    @yield_values = []
    @results = []
    @raises = []
  end

  def match?(*args)
    @args.nil? || args == @args
  end

  def execute
    @count += 1

    unless @raises.empty?
      raise @raises.size > 1 ? @raises[@count - 1] : @raises[0]
    end

    if block_given? && !@yield_values.empty?
      @yield_values.each { |yield_value| yield yield_value }
    elsif !@results.empty?
      @results.size > 1 ? @results[@count - 1] : @results[0]
    end
  end

  def with(*args)
    @args = args
    self
  end

  def and_return(*results)
    @results.concat(results)

    # Update count restrictions if results' size exceeds it
    if @count_restriction.is_a?(Range) && @count_restriction.end && @results.size > @count_restriction.end
      @count_restriction = (@count_restriction.first..@results.size)
    elsif @count_restriction.is_a?(Integer) && @results.size > @count_restriction
      @count_restriction = @results.size
    end

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
    case n
    when :once
      @count_restriction = 1..
    when :twice
      @count_restriction = 2..
    else
      @count_restriction = n..
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
    @count_restriction = nil
    self
  end

  def validate!
    unless @count_restriction == nil || @count_restriction === @count
      message = "#{@subject.inspect} should have received ##{@message}"
      if @count_restriction != nil
        message << " #{@count_restriction} time(s) but received #{@count} time(s)"
      end

      raise SpecFailedException, message
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
      raise SpecFailedException, "#{subject.inspect} should include #{value.inspect}" unless subject.include?(value)
    end
  end

  def inverted_match(subject)
    @values.each do |value|
      raise SpecFailedException, "#{subject.inspect} should not include #{value.inspect}" if subject.include?(value)
    end
  end
end

class HaveConstantExpectation
  def initialize(constant)
    @constant = constant.to_sym
  end

  def match(subject)
    unless subject.constants.include?(@constant)
      raise SpecFailedException, "Expected #{subject} to have constant '#{@constant}' but it does not"
    end
  end

  def inverted_match(subject)
    if subject.constants.include?(@constant)
      raise SpecFailedException, "Expected #{subject} NOT to have constant '#{@constant}' but it does"
    end
  end
end

class HaveMethodExpectation
  def initialize(method)
    @method = method.to_sym
  end

  def match(subject)
    unless subject.methods.include?(@method)
      raise SpecFailedException, "Expected #{subject} to have method '#{@method}' but it does not"
    end
  end

  def inverted_match(subject)
    if subject.methods.include?(@method)
      raise SpecFailedException, "Expected #{subject} NOT to have method '#{@method}' but it does"
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

  def be_positive_infinity
    BeInfinityExpectation.new(1)
  end

  def be_negative_infinity
    BeInfinityExpectation.new(-1)
  end

  def be_true
    EqualExpectation.new(true)
  end

  def be_false
    EqualExpectation.new(false)
  end

  def be_true_or_false
    TrueFalseExpectation.new()
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

  def output(expected)
    OutputExpectation.new(expected)
  end

  def output_to_fd(expected)
    OutputExpectation.new(expected)
  end

  def raise_error(klass = StandardError, message = nil, &block)
    RaiseErrorExpectation.new(klass, message, &block)
  end

  def complain(message = nil, verbose: false)
    ComplainExpectation.new(message, verbose: verbose)
  end

  def should_receive(message)
    Stub.new(self, message).tap { |stub| $stub_registry.register(stub) }
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

  def have_constant(method)
    HaveConstantExpectation.new(method)
  end

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
    Stub.new(self, message).any_number_of_times.tap { |stub| $stub_registry.register(stub) }
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
  $stub_registry = StubRegistry.new

  before_all_done = []
  after_all_done = []
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
        $stub_registry.reset
        $expectations = []
        context.each { |con| con.before_each.each { |b| b.call } }

        fn.call

        $expectations.each { |expectation| expectation.validate! }

      rescue SpecFailedException => e
        @failures << [context, test, e]
        formatter.print_failure(*@failures.last)
      rescue Exception => e
        raise if e.is_a?(SystemExit)
        @errors << [context, test, e]
        formatter.print_error(*@errors.last)
      else
        formatter.print_success(context, test)
      ensure
        # ensure that the after-each is executed
        context.each { |con| con.after_each.each { |a| a.call } }
      end


    else
      @skipped << [context, test]
      formatter.print_skipped(*@skipped.last)
    end
  end

  # after-all
  @specs.each do |test|
    context = test[0]
    context.each do |con|
      con.after_all.each do |b|
        unless after_all_done.include?(b)
          b.call
          after_all_done << b
        end
      end
    end
  end

  formatter.print_finish(@test_count, @failures, @errors, @skipped)
end

at_exit { run_specs }
