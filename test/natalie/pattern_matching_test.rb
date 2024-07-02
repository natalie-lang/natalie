require_relative '../spec_helper'

module PatternMatchingHelper
  def self.one = 1

  ABStruct = Struct.new(:a, :b)
end

# NATFIXME: Temporary file until we can run some things in `language/pattern_matching_spec.rb`
describe 'pattern matching' do
  it 'can assign a single value' do
    1 => a
    a.should == 1
  end

  it 'can assign a single value from an expression' do
    1 + 1 => a
    a.should == 2
  end

  it 'can assign a single value from a variable' do
    a = 1
    a => b
    b.should == 1
  end

  it 'can assign a single value from a method call' do
   PatternMatchingHelper.one => a
   a.should == 1
  end

  it 'can handle unnamed variable' do
    1 => _
  end

  it 'does not define a local variable if the expression fails' do
    -> {
      (raise RuntimeError, 'expected error'; 2) => a
    }.should raise_error(RuntimeError, 'expected error')
    -> { a }.should raise_error(NameError, /undefined (?:local variable or )?method `a' for main/)
  end

  it 'does not change an existing variable if the expression fails' do
    a = 1
    2 => a
    a.should == 2
    -> {
      (raise RuntimeError, 'expected error'; 3) => a
    }.should raise_error(RuntimeError, 'expected error')
    a.should == 2
  end

  it 'has no used mode, always returns nil' do
    # (1 => a).should is a parse error, so use a lambda instead
    l = -> { 1 => a }
    l.call.should be_nil
  end

  it 'can match against a constant read node' do
    -> { 1 => Integer }.should_not raise_error(NoMatchingPatternError)
  end

  it 'can match against a constant path node' do
    -> { 1.0 / 0.0 => Float::INFINITY }.should_not raise_error(NoMatchingPatternError)
  end

  it 'can match against anything that implements #===' do
    1 => 1
    -> { 1 => 2 }.should raise_error(NoMatchingPatternError, '1: 2 === 1 does not return true')

    1 => (1..10)
    -> { 1 => (2..10) }.should raise_error(NoMatchingPatternError, '1: 2..10 === 1 does not return true')
  end

  it 'has no used mode, always returns nil' do
    # (1 => Integer).should is a parse error, so use a lambda instead
    l = -> { 1 => Integer }
    l.call.should be_nil
  end

  it 'will raise NoMatchingPatternError on invalid match with the constant' do
    -> { 1 => String }.should raise_error(NoMatchingPatternError, '1: String === 1 does not return true')
  end

  it 'will raise NoMatchingPatternError on invalid match with the constant path' do
    -> { 1 => Float::INFINITY }.should raise_error(NoMatchingPatternError, '1: Infinity === 1 does not return true')
  end

  it 'has the correct scoping for the lhs' do
    a = 1
    (b = 2; a + b) => Integer
    a.should == 1
    b.should == 2
  end

  it 'evaluates the lhs only once and uses that value in the error message' do
    a = 1
    -> { a += 1 => Float::INFINITY }.should raise_error(NoMatchingPatternError, '2: Infinity === 2 does not return true')
    a.should == 2
  end

  it 'can handle array targets with fixed size and only local variable target nodes' do
    [1, 2, 3] => a, b, c
    a.should == 1
    b.should == 2
    c.should == 3
  end

  it 'can recursively handle local variables' do
    x = 1
    y = 2
    -> {
      [x, x + y] => a, b
      a.should == 1
      b.should == 3
    }.call
  end

  it 'can handle array targets with any input that implements #deconstruct' do
    deconstruct = mock('deconstruct')
    deconstruct.should_receive(:deconstruct).and_return([1, 2])
    deconstruct => a, b
    a.should == 1
    b.should == 2
  end

  it 'can handle unnamed variables' do
    [1, 2, 3] => a, _, _
    a.should == 1
  end

  it 'raises an exception for input that does not implement #deconstruct' do
    -> {
      1 => a, b
    }.should raise_error(NoMatchingPatternError, '1: 1 does not respond to #deconstruct')
  end

  it 'raises an exception for array target with invalid size' do
    -> {
      [1, 2] => a, b, c
    }.should raise_error(NoMatchingPatternError, '[1, 2]: [1, 2] length mismatch (given 2, expected 3)')
  end

  it 'uses both the original input and the result of #deconstruct in the error message' do
    s = PatternMatchingHelper::ABStruct.new(1, 2)
    -> {
      s => a, b, c
    }.should raise_error(NoMatchingPatternError, "#{s}: [1, 2] length mismatch (given 2, expected 3)")
  end

  it 'can handle unnamed splat output without additional fields' do
    [1, 2] => [a, b, *]
    a.should == 1
    b.should == 2
  end

  it 'can handle unnamed splat output with additional fields' do
    [1, 2, 3, 4] => [a, b, *]
    a.should == 1
    b.should == 2
  end

  it 'can handle unnamed splat as only target' do
    [1, 2, 3, 4] => [*]
  end

  it 'raises an exception if the input is too short' do
    -> {
      [1, 2] => [a, b, c, *]
    }.should raise_error(NoMatchingPatternError, '[1, 2]: [1, 2] length mismatch (given 2, expected 3+)')
  end

  it 'can handle named splat output without additional fields' do
    [1, 2] => [a, b, *c]
    a.should == 1
    b.should == 2
    c.should == []
  end

  it 'can handle named splat output with additional fields' do
    [1, 2, 3, 4] => [a, b, *c]
    a.should == 1
    b.should == 2
    c.should == [3, 4]
  end

  it 'can handle named splat as only target' do
    [1, 2, 3, 4] => [*c]
    c.should == [1, 2, 3, 4]
  end

  it 'raises an exception if the input is too short' do
    -> {
      [1, 2] => [a, b, c, *d]
    }.should raise_error(NoMatchingPatternError, '[1, 2]: [1, 2] length mismatch (given 2, expected 3+)')
  end

  it 'can handle post targets' do
    [1, 2, 3, 4] => [a, *b, c]
    a.should == 1
    b.should == [2, 3]
    c.should == 4
  end

  it 'can handle unnamed variables' do
    [1, 2, 3, 4] => [_, *_, a]
    a.should == 4
  end

  it 'raises an exception if the input is too short' do
    -> {
      [1, 2] => [a, b, *c, d]
    }.should raise_error(NoMatchingPatternError, '[1, 2]: [1, 2] length mismatch (given 2, expected 3+)')
  end

  it 'supports non-assignments in fixed size match lists' do
    -> {
      [1, 2] => 1, 2
    }.should_not raise_error(NoMatchingPatternError)
  end

  it 'can combine match and set in fixed size match lists' do
    [1, 2, 3] => 1, a, 3
    a.should == 2
  end

  it 'raises an exception if the pattern does not match in fixed size match lists' do
    -> {
      [1, 2] => 1, 3
    }.should raise_error(NoMatchingPatternError, '[1, 2]: 3 === 2 does not return true')
  end

  it 'raises an exception if the pattern does not match in combined match and set in fixed size match lists' do
    -> {
      [1, 2, 3] => 1, a, 2
    }.should raise_error(NoMatchingPatternError, '[1, 2, 3]: 2 === 3 does not return true')
  end

  it 'raises an exception if the pattern does not match in combined match and set in fixed size match lists if the size does not match' do
    -> {
      [1, 2, 3] => 1, a, 3, 4
    }.should raise_error(NoMatchingPatternError, '[1, 2, 3]: [1, 2, 3] length mismatch (given 3, expected 4)')
  end

  it 'can combine match and set in match lists with rest' do
    [1, 2, 3, 4] => 1, a, *b
    a.should == 2
    b.should == [3, 4]
  end

  it 'raises an exception if the pattern does not match in match lists with rest' do
    -> {
      [1, 2] => [1, 3, *]
    }.should raise_error(NoMatchingPatternError, '[1, 2]: 3 === 2 does not return true')
  end

  it 'raises an exception if the pattern does not match in combined match and set in match lists with rest' do
    -> {
      [1, 2, 3, 4] => [1, *, 2]
    }.should raise_error(NoMatchingPatternError, '[1, 2, 3, 4]: 2 === 4 does not return true')
  end

  it 'raises an exception if the pattern does not match in combined match and set in match lists with rest if the size does not match' do
    -> {
      [1, 4] => 1, *a, 3, 4
    }.should raise_error(NoMatchingPatternError, '[1, 4]: [1, 4] length mismatch (given 2, expected 3+)')
  end

  it 'can handle array targets with a constant' do
    s = PatternMatchingHelper::ABStruct.new(1, 2)
    s => PatternMatchingHelper::ABStruct[a, b]
    a.should == 1
    b.should == 2
  end

  it 'matches Struct constant for any Struct class instance' do
    s = PatternMatchingHelper::ABStruct.new(1, 2)
    s => Struct[a, b]
    a.should == 1
    b.should == 2
  end

  it 'raises the correct error message for invalid classes' do
    s = Struct.new(:b, :c).new(1, 2)
    -> {
      s => PatternMatchingHelper::ABStruct[_, _]
    }.should raise_error(NoMatchingPatternError, "#{s}: PatternMatchingHelper::ABStruct === #{s} does not return true")
  end

  it 'raises the correct error message for invalid pattern size' do
    s = PatternMatchingHelper::ABStruct.new(1, 2)
    -> {
      s => PatternMatchingHelper::ABStruct[_]
    }.should raise_error(NoMatchingPatternError, "#{s}: [1, 2] length mismatch (given 2, expected 1)")
  end

  it 'raises the correct error message for invalid patterns' do
    s = PatternMatchingHelper::ABStruct.new(1, 2)
    -> {
      s => PatternMatchingHelper::ABStruct[1, 3]
    }.should raise_error(NoMatchingPatternError, "#{s}: 3 === 2 does not return true")
  end

  it 'supports pinned variables' do
    a = 1
    1 => ^a
  end

  it 'generated the correct error message for pinned variables' do
    a = 2
    -> {
      1 => ^a
    }.should raise_error(NoMatchingPatternError, '1: 2 === 1 does not return true')
  end

  it 'supports pinned expressions' do
    a = 1
    2 => ^(a + 1)
  end

  it 'generated the correct error message for pinned expressions' do
    a = 1
    -> {
      1 => ^(a + a)
    }.should raise_error(NoMatchingPatternError, '1: 2 === 1 does not return true')
  end

  it 'evaluates a pinned expression only once' do
    a = 1
    -> {
      1 => ^(a += a)
    }.should raise_error(NoMatchingPatternError, '1: 2 === 1 does not return true')
    a.should == 2
  end
end

describe 'NoMatchingPatternError' do
  it 'is a subclass of StandardError' do
    NoMatchingPatternError.superclass.should == StandardError
  end
end
