require_relative '../spec_helper'

# NATFIXME: Temprorary file until we can run some things in `language/pattern_matching_spec.rb`
describe 'pattern matching' do
  it 'can assign a single value' do
    1 => a
    a.should == 1
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
end

describe 'NoMatchingPatternError' do
  it 'is a subclass of StandardError' do
    NoMatchingPatternError.superclass.should == StandardError
  end
end
