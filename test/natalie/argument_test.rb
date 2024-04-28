require_relative '../spec_helper'

def argument_proxy(*args, **kwargs)
  [args, kwargs]
end

def underscore_arguments(_, _) _ end

def optional_splat_keyword_arguments(o = 1, *s, k:)
  [o, s, k]
end

def optional_keyword_keyword_rest_arguments(o = 1, k: 2, **kr)
  [o, k, kr]
end

describe 'splat operators' do
  it 'should work with literal arguments' do
    argument_proxy(1).should == [[1], {}]
    argument_proxy(1, 2).should == [[1, 2], {}]
    argument_proxy(1, 2, {}).should == [[1, 2, {}], {}]
  end

  it 'should work with literal keyword arguments' do
    argument_proxy(foo: 'a').should == [[], { foo: 'a' }]
    argument_proxy(foo: 'a', bar: 'b').should == [[], { foo: 'a', bar: 'b' }]
  end

  it 'should work with literal positional and keyword arguments' do
    argument_proxy(1, foo: 'a').should == [[1], { foo: 'a' }]
    argument_proxy(1, 2, foo: 'a', bar: 'b').should == [[1, 2], { foo: 'a', bar: 'b' }]
  end

  it 'should work with splat operators' do
    args = [1, 2]
    argument_proxy(*args).should == [[1, 2], {}]
    argument_proxy(*[1, 2]).should == [[1, 2], {}]
    args << {}
    argument_proxy(*args).should == [[1, 2, {}], {}]
    argument_proxy(*[1, 2, {}]).should == [[1, 2, {}], {}]
  end

  it 'should work with splat operator and literal keywords' do
    args = [1, 2]
    argument_proxy(*args, foo: 'a').should == [[1, 2], { foo: 'a' }]
    argument_proxy(*[1, 2], foo: 'a').should == [[1, 2], { foo: 'a' }]
  end

  it 'should work with splat operator and double splat operator' do
    args = [1, 2]
    kwargs = { foo: 'a' }
    argument_proxy(*args, **kwargs).should == [[1, 2], { foo: 'a' }]
    argument_proxy(*[1, 2], **kwargs).should == [[1, 2], { foo: 'a' }]
    argument_proxy(*args, **{ foo: 'a' }).should == [[1, 2], { foo: 'a' }]
    argument_proxy(*[1, 2], **{ foo: 'a' }).should == [[1, 2], { foo: 'a' }]
  end

  it 'should work with literal positional arguments and double splat operator' do
    kwargs = { foo: 'a' }
    argument_proxy(1, 2, **kwargs).should == [[1, 2], { foo: 'a' }]
    argument_proxy(1, 2, **{ foo: 'a' }).should == [[1, 2], { foo: 'a' }]
  end
end

describe 'forward args' do
  def bar(a, b:, c: nil, &block)
    [a, b, c, block&.call].compact
  end

  def foo(...)
    baz(bar(...))
  end

  def baz(a)
    a
  end

  it 'passes all arguments as-is' do
    foo(1, b: 2, c: 3).should == [1, 2, 3]
  end

  it 'passes block arguments' do
    foo(1, b: 2) { 4 }.should == [1, 2, 4]
  end
end

describe 'underscore args' do
  underscore_arguments(1, 2).should == 1
end

describe 'keyword argument following splat and optional' do
  optional_splat_keyword_arguments(:a, :b, k: :c).should == [:a, [:b], :c]
end

describe 'optional and keyword argument followed by keyword rest' do
  optional_keyword_keyword_rest_arguments(:a, k: :b, c: :d).should == [:a, :b, { c: :d }]
end

describe 'implicit rest arg' do
  [[1, 2, 3]].map { |x,| x }.should == [1]
end

describe 'numbered arguments' do
  it 'can be used in lambdas' do
    sum = -> { _1 + _2 }
    sum.call(1, 2).should == 3
  end

  it 'can be used in procs' do
    sum = proc { _1 + _2 }
    sum.call(1, 2).should == 3
  end

  it 'preserves the arity' do
    sum = -> { _1 + _2 }
    sum.arity.should == 2
  end

  it 'preserves the arity even if arguments are skipped' do
    sum = -> { _1 + _3 }
    sum.arity.should == 3
  end
end
